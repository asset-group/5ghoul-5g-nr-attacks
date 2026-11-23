cmake_minimum_required(VERSION 3.18.4)
# ---------------------------------------------------------------------------
# Toolchain note (issue #46, WSL2 / Ubuntu 26.04)
#
# The compiler is conda-forge clang, selected by major version through the
# WDS_CLANG_MAJOR cache variable (default 23 = clang/LLD/LLVM 23.1.6, the
# newest release published on conda-forge).  15.0.7 is the version this tree
# was first proven with: pass -DWDS_CLANG_MAJOR=15 to reproduce that build.
#
# Two linker facts drive the code below:
#  * conda-forge's clang ships no LLVMgold.so, so GNU ld cannot load the
#    thin-LTO plugin and every CMake probe link fails:
#        .../bin/x86_64-conda-linux-gnu-ld: .../lib/LLVMgold.so:
#        error loading plugin: cannot open shared object file
#    A failing probe link makes check_include_file()/check_c_source_runs()
#    report "not found", which is what aborted configure at Wireshark's
#    HAVE_C99_VSNPRINTF test (libs/wireshark/ConfigureChecks.cmake:139).
#    With lld the identical probe compiles, links and exits 0.
#  * lld <= 19 is installed by conda-forge as lld-<major>, and the driver
#    accepts -fuse-ld=lld-<major>.  lld 20+ ships only ld.lld/lld and clang
#    rejects "-fuse-ld=lld-23" with "invalid linker name", so the name is
#    resolved from what is installed instead of being hardcoded.
#
# llvm-tools supplies the bitcode-aware llvm-ar/llvm-ranlib that thin-LTO
# archives require (conda's gcc-ar is plugin-less).
# ---------------------------------------------------------------------------
set(WDS_CLANG_MAJOR "23" CACHE STRING
  "conda-forge clang major version to build with (23 = latest, 15 = original)")
set(CMAKE_C_COMPILER "clang-${WDS_CLANG_MAJOR}")
set(CMAKE_CXX_COMPILER "clang++-${WDS_CLANG_MAJOR}")

project(
  wdissector
  LANGUAGES CXX C
  VERSION 1.0)
include(Modules.cmake)
include(FetchContent)
include(ExternalProject)
set(WIRESHARK_VERSION "3.7.0")
set(WIRESHARK_BIN_DIR "${PROJECT_SOURCE_DIR}/bin")
log(INFO "Current Generator: ${CMAKE_GENERATOR}")
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1
  CACHE INTERNAL "No dev warnings")

# Wireshark source path
set(WS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/libs/wireshark)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${WS_SOURCE_DIR}/cmake/modules
  ${PROJECT_SOURCE_DIR}/libs/cef-cmake/cmake)

# ---------------- System Dependencies --------------------
find_program(ZSTD_EXIST "zstd" REQUIRED)
find_program(FUSER_EXIST "fuser" REQUIRED)
find_program(SSHPASS_EXIST "sshpass" REQUIRED)
find_program(SWIG_EXIST "swig")
find_program(PKG-CONFIG_EXIST "pkg-config" REQUIRED)
find_program(FIND_EXIST "find" REQUIRED)

# * pthreads
find_package(Threads REQUIRED)

# * glib-2.0
find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

if(GLIB_FOUND)
  log(INFO "Found GLIB-2.0 library")
  log(STATUS "LD Flags: ${GLIB_LDFLAGS}")
  log(STATUS "C Flags: ${GLIB_CFLAGS}")
endif()

# * freetype2
find_package(Freetype REQUIRED)

if(FREETYPE_FOUND)
  log(INFO "Found FreeType2 ${FREETYPE_VERSION_STRING} library")
  log(STATUS "LD Flags: ${FREETYPE_LDFLAGS}")
  log(STATUS "C Flags: ${FREETYPE_CFLAGS}")
endif()

# zlib
find_package(ZLIB REQUIRED)

if(ZLIB_FOUND)
  log(INFO "Found ZLIB library")
  log(STATUS "${ZLIB_LIBRARY}")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++2a" COMPILER_SUPPORTS_CXX20)

if(NOT COMPILER_SUPPORTS_CXX20)
  message(FATAL_ERROR "Compiler ${CC} does not support C++20")
endif()

# ---------------- Configuration --------------------
# Options
set(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(WIRESHARK_GUI
  ON
  CACHE BOOL "Compile Wireshark GUI")
set(CMAKE_EXPORT_COMPILE_COMMANDS
  ON
  CACHE BOOL "Export compile_commands.json" FORCE)
set(ENABLE_EXPORTS ON)
set(FETCHCONTENT_QUIET FALSE)
option(FORCE_COLORED_OUTPUT
  "Always produce ANSI-colored output (GNU/Clang only)." TRUE)

if(${FORCE_COLORED_OUTPUT})
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
  endif()
endif()

# Configure Output folders
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# Global compiler options
# C++20 is required (std::unordered_map::contains, if constexpr on concepts).
# The toolchain deliberately uses libstdc++ 12 headers: GCC 13 would add
# std::format, which makes the unqualified format(...) calls in
# Machine/Framework/Modules ambiguous against the vendored fmt::format via ADL,
# while GCC 9's <valarray> has a noexcept mismatch that clang rejects in C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(COMMON_FLAGS
    "-fno-stack-protector -fpic -Wall -Wno-braced-scalar-init -Wno-unused-variable -Wno-reorder -flto=thin"
  )
else()
  set(COMMON_FLAGS
    "-fno-stack-protector -fpic -Wall -Wno-braced-scalar-init -Wno-unused-variable -Wno-reorder"
  )
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_BUILD_RPATH $ORIGIN bin)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")

# Configure linker (enforce lld from the same LLVM release as clang)
find_program(WDS_LLD_VERSIONED NAMES lld-${WDS_CLANG_MAJOR} ld.lld-${WDS_CLANG_MAJOR})
if(WDS_LLD_VERSIONED)
  set(WDS_LLD_NAME "lld-${WDS_CLANG_MAJOR}")
else()
  # lld 20+ installs only ld.lld/lld; -fuse-ld=lld resolves ld.lld in PATH.
  find_program(WDS_LLD_UNVERSIONED NAMES ld.lld lld REQUIRED)
  set(WDS_LLD_NAME "lld")
endif()
log(INFO "Linker: -fuse-ld=${WDS_LLD_NAME} (${WDS_LLD_VERSIONED}${WDS_LLD_UNVERSIONED})")
add_link_options("-fuse-ld=${WDS_LLD_NAME}")

# Probes must link with lld as well.  add_link_options() above only applies to
# targets; CMake's try_compile()/check_* probes use CMAKE_*_LINKER_FLAGS.  With
# GNU ld those probe links fail (LLVMgold.so missing), which silently turns
# every ConfigureChecks result into "not found" -- including Wireshark's C99
# vsnprintf requirement at libs/wireshark/ConfigureChecks.cmake:139.
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=${WDS_LLD_NAME}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=${WDS_LLD_NAME}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=${WDS_LLD_NAME}")

# Thin-LTO object files must be archived/indexed by LLVM's ar, not conda's
# gcc-ar (also plugin-less).  llvm-tools provides both, versioned or plain.
find_program(LLVM_AR_EXECUTABLE NAMES llvm-ar-${WDS_CLANG_MAJOR} llvm-ar)
find_program(LLVM_RANLIB_EXECUTABLE NAMES llvm-ranlib-${WDS_CLANG_MAJOR} llvm-ranlib)
if(LLVM_AR_EXECUTABLE)
  set(CMAKE_AR "${LLVM_AR_EXECUTABLE}")
endif()
if(LLVM_RANLIB_EXECUTABLE)
  set(CMAKE_RANLIB "${LLVM_RANLIB_EXECUTABLE}")
endif()

# Print some options
log(INFO "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
log(INFO "EXPORT_COMPILE_COMMANDS: ${EXPORT_COMPILE_COMMANDS}")

# ---------------- Includes --------------------
# Global Includes
include_directories(
  AFTER ${PROJECT_SOURCE_DIR}/ ${PROJECT_SOURCE_DIR}/src/
  ${PROJECT_SOURCE_DIR}/libs/)

# WDissector Includes
set(WDISSECTOR_INCLUDES
  ${PROJECT_SOURCE_DIR}/
  ${PROJECT_SOURCE_DIR}/src/
  ${PROJECT_SOURCE_DIR}/libs/
  ${GLIB_INCLUDE_DIRS}
  ${WS_SOURCE_DIR}/
  ${WS_SOURCE_DIR}/include
  ${WS_SOURCE_DIR}/epan
  ${WS_SOURCE_DIR}/epan/ftypes
  libs/serialport/include/
  libs/PcapPlusPlus/Common++/header/
  libs/PcapPlusPlus/Packet++/header/
  libs/PcapPlusPlus/Pcap++/header/)

# ---------------- Libraries --------------------

# Download submodules and apply patch to their source code
execute_process(
  COMMAND bash -c "./scripts/apply_patches.sh ./libs && ./scripts/apply_patches.sh ./modules"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# libPCAP from the conda env (1.10.6).  The vendored tree used to provide the
# `pcap` target that wireapi links, so recreate it as an INTERFACE target.
find_path(PCAP_INCLUDE_DIR NAMES pcap.h PATHS ${CMAKE_PREFIX_PATH} PATH_SUFFIXES include)
find_library(PCAP_LIBRARY NAMES pcap PATHS ${CMAKE_PREFIX_PATH} PATH_SUFFIXES lib)
if(NOT PCAP_INCLUDE_DIR OR NOT PCAP_LIBRARY)
  message(FATAL_ERROR
    "conda libpcap not found (pcap.h / libpcap.so) under ${CMAKE_PREFIX_PATH}")
endif()
# IMPORTED, not INTERFACE: wireshark's CMake exports its targets and an
# INTERFACE library would have to join that export set, which fails with
# "export called with target caputils which requires target pcap that is not in
# any export set".  Imported targets are not exported and need no set.
add_library(pcap UNKNOWN IMPORTED)
set_target_properties(pcap PROPERTIES
  IMPORTED_LOCATION "${PCAP_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${PCAP_INCLUDE_DIR}")
set(HAVE_LIBPCAP 1)
set(ENABLE_PCAP ON)
set(CAN_SET_CAPTURE_BUFFER_SIZE 1)
set(PCAP_FOUND ON)

# GLFW3 from the conda env (3.4, the exact version the vendored tree shipped).
# glfw3Config exports the `glfw` target that imgui links.
find_package(glfw3 CONFIG REQUIRED)

# Configure Pagmo (conda-forge, header-only; there is no vendored copy in-tree)
log(INFO "Using conda-forge pagmo (header-only)")

# Boost comes from the conda env (1.90.0).  The vendored 1.78 tree that used
# to be downloaded and b2-built here was dead weight: nothing linked it, and the
# .so files it copied into bin/ were never resolved (ldd shows conda's 1.90.0).
find_package(Boost 1.51.0 MODULE
  COMPONENTS
  context
  filesystem
  program_options
  regex
  system
  thread
  serialization
  iostreams
  REQUIRED
)

# 4) Pagmo: conda-forge's pagmo 2.9 is the only copy; no vendored tree is kept.
#
# conda-forge pagmo >= 2.20 is NOT header-only any more: pagmo::population,
# pso_gen::evolve and get_extra_info live in libpagmo.so, and leaving them out
# produces "undefined symbol: pagmo::population::population(...)" at link time.
# The distributed pagmo-config.cmake is still deliberately NOT used: that config
# runs a pre-Eigen-3.4 FindEigen3 (it greps EIGEN_WORLD_VERSION, removed in
# Eigen 3.4, and this env has Eigen 5.0.1) and it hard-requires IPOPT/NLOPT
# module files we do not need.  So headers are found by hand and the shared
# library is linked directly.
find_path(PAGMO_INCLUDE_DIR
  NAMES pagmo/pagmo.hpp
  PATHS ${CMAKE_PREFIX_PATH}
  PATH_SUFFIXES include)
if(NOT PAGMO_INCLUDE_DIR)
  message(FATAL_ERROR
    "conda pagmo headers not found (<prefix>/include/pagmo/pagmo.hpp) under "
    "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}. Install 'pagmo' into the conda env.")
endif()
find_library(PAGMO_LIBRARY
  NAMES pagmo
  PATHS ${CMAKE_PREFIX_PATH}
  PATH_SUFFIXES lib)
if(NOT PAGMO_LIBRARY)
  message(FATAL_ERROR
    "conda libpagmo not found (<prefix>/lib/libpagmo.so) under "
    "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}. pagmo >= 2.20 is not header-only.")
endif()
log(INFO "conda pagmo headers: ${PAGMO_INCLUDE_DIR}")
log(INFO "conda pagmo library: ${PAGMO_LIBRARY}")
add_library(pagmo INTERFACE)
target_include_directories(pagmo INTERFACE ${PAGMO_INCLUDE_DIR})
# Boost.Serialization is named explicitly because 5g_fuzzer archives
# fitness_session_t with it; libpagmo itself pulls nlopt/ipopt/tbb via its
# RUNPATH deps in the same conda prefix.
target_link_libraries(pagmo INTERFACE ${PAGMO_LIBRARY}
  Boost::iostreams Boost::serialization)

# Chrome Embbeded Framework (CEF) library
set(CEF_USE_SANDBOX
  OFF
  CACHE BOOL "Force turning off of sandbox")
include(cef_cmake)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/cef-cmake)

# Cross-platform serial library
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/serialport)

# Graphviz C++ Library (gvpp target)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/gvpp)

# tinyprocess (process library)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/tinyprocess)

# react-cpp (event loop library)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/react-cpp/)

# Pcap PlusPlus
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/PcapPlusPlus/)
set(PcapPlusPlus CommonPP PacketPP PcapPP)
include_directories(
  libs/PcapPlusPlus/Pcap++/header/
  libs/PcapPlusPlus/Packet++/header/
  libs/PcapPlusPlus/Common++/header/)

# RTAudio -- conda-forge librtaudio 5.2.0. The package ships no CMake config module
# (only rtaudio.pc), so import the shared library by hand under the name the link
# lists already use. Gist (audio feature extraction) lived inside the old vendored
# rtaudio tree and is still needed by MonitorMicrophone, so it stays vendored.
find_library(RTAUDIO_LIBRARY NAMES rtaudio REQUIRED)
find_path(RTAUDIO_INCLUDE_DIR NAMES rtaudio/RtAudio.h REQUIRED)
add_library(rtaudio UNKNOWN IMPORTED)
set_target_properties(rtaudio PROPERTIES
  IMPORTED_LOCATION "${RTAUDIO_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${RTAUDIO_INCLUDE_DIR}")
message(STATUS "rtaudio: ${RTAUDIO_LIBRARY} (headers ${RTAUDIO_INCLUDE_DIR})")
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/gist)

# PyBind11 (Run Python Modules from C++)
#
# The interpreter is the conda env's Python, not a bundled standalone build.
# This used to download cpython 3.8.16 into modules/python, pip-install
# modules/requirements.txt into it and pin PYTHON_* at that tree.  The runtime
# dependencies now live in the env's site-packages instead:
#     python3 -m pip install -r modules/requirements.txt
# (done once per env; see logs/py311-pip-install.log)
find_package(Python3 3.11 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "python: ${Python3_EXECUTABLE} (${Python3_VERSION}, headers ${Python3_INCLUDE_DIRS})")

set(PYBIND11_PYTHONLIBS_OVERWRITE OFF)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
set(PYTHON_INCLUDE_DIR ${Python3_INCLUDE_DIRS})
set(PYTHON_LIBDIR ${Python3_LIBRARY_DIRS})
set(PYTHON_LIBRARY ${Python3_LIBRARIES})

# Absolute interpreter path for the places that spawn python as a subprocess
# (Framework.hpp WD_PYTHON, QCMonitor/QCSuper).
add_compile_definitions(WD_PYTHON_DEFAULT="${Python3_EXECUTABLE}")

add_subdirectory(${PROJECT_SOURCE_DIR}/libs/pybind11/)
target_link_directories(pybind11::embed INTERFACE
  ${Python3_LIBRARY_DIRS})

# Dynamo (shared library loader)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/dynalo/)

# libzmq and cppzmq come from the conda env (zeromq 4.3.5, cppzmq 4.11.0).
# ZeroMQConfig exports the `libzmq` target, the same name FUZZER_LIBS links.
find_package(ZeroMQ CONFIG REQUIRED)

# double-conversion used to be vendored for the in-tree folly build.  conda
# folly links its own double-conversion 3.4.0 from the env, and nothing in this
# project links the vendored copy, so the subdirectory is gone.

# fmt and folly come from the conda env: fmt 12.1.0 and folly 2026.10.26.00.
# conda's libfolly is compiled against fmt 12, so the vendored fmt 9.1.1 tree
# cannot be mixed with it in the same translation units.  Both packages are
# already present in env 5ghoul23 and satisfy each other's pins
# (folly: fmt >=12.1.0,<12.2.0, libboost >=1.90,<1.91, glog >=0.7.1,<0.8,
#  libstdcxx >=15 -- the env has libstdcxx 16.2.0).
find_package(fmt CONFIG REQUIRED)
find_package(folly CONFIG REQUIRED)
log(INFO "conda fmt:   ${fmt_VERSION}")
log(INFO "conda folly: ${FOLLY_INCLUDE_DIR}")

# Build Wireshark if needed
if(NOT WIRESHARK_GUI)
  # Disable Wireshark GUI by default
  set(ENABLE_LUA
    ON
    CACHE BOOL "")
  set(ENABLE_STATIC
    OFF
    CACHE BOOL "")
  set(BUILD_wireshark
    OFF
    CACHE BOOL "")
  set(BUILD_fuzzshark
    OFF
    CACHE BOOL "")
  set(BUILD_androiddump
    OFF
    CACHE BOOL "")
  set(BUILD_capinfos
    OFF
    CACHE BOOL "")
  set(BUILD_captype
    OFF
    CACHE BOOL "")
  set(BUILD_ciscodump
    OFF
    CACHE BOOL "")
  set(BUILD_dcerpcidl2wrs
    OFF
    CACHE BOOL "")
  set(BUILD_dftest
    OFF
    CACHE BOOL "")
  set(BUILD_dpauxmon
    OFF
    CACHE BOOL "")
  set(BUILD_dumpcap
    OFF
    CACHE BOOL "")
  set(BUILD_editcap
    OFF
    CACHE BOOL "")
  set(BUILD_mergecap
    OFF
    CACHE BOOL "")
  set(BUILD_mmdbresolve
    OFF
    CACHE BOOL "")
  set(BUILD_randpkt
    OFF
    CACHE BOOL "")
  set(BUILD_randpktdump
    OFF
    CACHE BOOL "")
  set(BUILD_rawshark
    OFF
    CACHE BOOL "")
  set(BUILD_reordercap
    OFF
    CACHE BOOL "")
  set(BUILD_sdjournal
    OFF
    CACHE BOOL "")
  set(BUILD_sharkd
    OFF
    CACHE BOOL "")
  set(BUILD_sshdump
    OFF
    CACHE BOOL "")
  set(BUILD_text2pcap
    OFF
    CACHE BOOL "")
  set(BUILD_tfshark
    OFF
    CACHE BOOL "")
  set(BUILD_tshark
    OFF
    CACHE BOOL "")
  set(BUILD_udpdump
    OFF
    CACHE BOOL "")
else()
  set(ENABLE_LUA
    ON
    CACHE BOOL "")
  set(ENABLE_STATIC
    OFF
    CACHE BOOL "")
endif()

log(INFO "Building custom Wireshark library")
add_subdirectory(${WS_SOURCE_DIR})

# Build oai_tracing folder
add_library(
  tracing STATIC
  libs/oai_tracing/database.c libs/oai_tracing/utils.c
  libs/oai_tracing/handler.c libs/oai_tracing/event.c libs/oai_tracing/config.c)
target_compile_options(tracing PRIVATE "-w")

# Build wireapi.a
add_library(
  wireapi STATIC
  libs/whereami.c
  libs/profiling.c
  ${WS_SOURCE_DIR}/ui/cmdarg_err.c
  ${WS_SOURCE_DIR}/ui/clopts_common.c
  ${WS_SOURCE_DIR}/ui/util.c
  ${WS_SOURCE_DIR}/ui/dissect_opts.c
  ${WS_SOURCE_DIR}/ui/failure_message.c
  ${WS_SOURCE_DIR}/ui/iface_toolbar.c
  ${WS_SOURCE_DIR}/ui/decode_as_utils.c
  ${WS_SOURCE_DIR}/ui/version_info.c
  ${WS_SOURCE_DIR}/frame_tvbuff.c
  ${WS_SOURCE_DIR}/capture/capture-pcap-util.c
  ${WS_SOURCE_DIR}/capture/capture-pcap-util-unix.c
  ${WS_SOURCE_DIR}/cfile.c
  ${WS_SOURCE_DIR}/file.c
  ${WS_SOURCE_DIR}/file_packet_provider.c
  ${WS_SOURCE_DIR}/extcap.c
  ${WS_SOURCE_DIR}/extcap_parser.c
  ${WS_SOURCE_DIR}/wsutil/please_report_bug.c)
target_compile_options(wireapi PRIVATE ${GLIB_CFLAGS} -w -O3)
target_include_directories(wireapi PUBLIC ${WDISSECTOR_INCLUDES})
target_link_libraries(
  wireapi
  PUBLIC ${GLIB_LDFLAGS}
  epan
  wiretap
  wsutil
  pcap
  -lgmodule-2.0
  -lz
  -lm
  -lpcre2-8)

add_dependencies(wireapi plugins)

# Build libwdissector_static.a
add_library(wdissector_static STATIC src/wdissector.cpp)
target_link_libraries(wdissector_static PUBLIC wireapi)
target_compile_options(wdissector_static PRIVATE "-DWDISSECTOR_STATIC_LIB")
set_target_properties(wdissector_static PROPERTIES CXX_VISIBILITY_PRESET default
  VISIBILITY_INLINES_HIDDEN 0)

# Build libwdissector.so
add_library(wdissector SHARED src/wdissector.cpp)
target_link_libraries(wdissector PUBLIC wireapi)
target_compile_options(wdissector PRIVATE "-DWDISSECTOR_STATIC_LIB")
set_target_properties(wdissector PROPERTIES CXX_VISIBILITY_PRESET default
  VISIBILITY_INLINES_HIDDEN 0)

# Build imgui
add_library(
  imgui STATIC
  libs/imgui/imgui_impl_glfw.cpp
  libs/imgui/imgui_impl_opengl3.cpp
  libs/imgui/libs/gl3w/GL/gl3w.c
  libs/imgui/stb_image.cpp
  libs/imgui/imgui.cpp
  libs/imgui/imgui_draw.cpp
  libs/imgui/imgui_tables.cpp
  libs/imgui/imgui_widgets.cpp
  libs/imgui/misc/freetype/imgui_freetype.cpp
  libs/imgui/misc/fonts/DroidSans.cpp
  libs/imgui/misc/fonts/DroidSansMono.cpp
  libs/imgui/misc/fonts/IconsFontAwesome5.cpp
  libs/imgui/imguial_term.cpp
  libs/imgui/ImGuiFileDialog/ImGuiFileDialog.cpp # ImGuiFileDialog
  libs/imgui/implot/implot.cpp # ImPlot
  libs/imgui/implot/implot_items.cpp # ImPlot
)
target_compile_options(imgui PRIVATE -w -O3)
target_compile_definitions(imgui PRIVATE -DIMGUI_IMPL_OPENGL_LOADER_GL3W)
target_include_directories(
  imgui
  PUBLIC libs/imgui/libs/gl3w/ libs/imgui/ ${FREETYPE_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/ ${PROJECT_SOURCE_DIR}/libs
  $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(imgui PRIVATE GL glfw rt m dl ${FREETYPE_LIBRARIES})

add_library(nanolog STATIC libs/NanoLog.cpp)
target_include_directories(nanolog PRIVATE ${PROJECT_SOURCE_DIR}/)

# libmpsse-static used to be declared here.  It had no consumer in any target
# (CMakeLists, src, modules, bindings) and produced zero build edges, so the
# dead IMPORTED target and the 14 MB tree are gone.

# ---------------- Targets --------------------
# Targets are compiled with shared libraries associated with wireapi.a
# automatically

# Shared memory interface library
add_library(shm_interface STATIC src/drivers/shm_interface/wd_shm.c)
target_include_directories(shm_interface PUBLIC src/drivers/shm_interface/)

# example_wdissector
add_executable(example_wdissector src/wdissector.cpp)
target_link_libraries(example_wdissector PRIVATE wireapi pthread)

# macpdu2wireshark
add_executable(macpdu2wireshark src/macpdu2wireshark.c)
target_link_libraries(macpdu2wireshark PRIVATE tracing wdissector_static pthread)
target_compile_options(macpdu2wireshark PRIVATE "-w")

# Common Fuzzer libs
set(FUZZER_LIBS
  ${CMAKE_THREAD_LIBS_INIT}
  wdissector_static
  fmt::fmt
  pagmo
  imgui
  Folly::folly
  shm_interface
  gvc # From graphviz
  cgraph # From graphviz
  libzmq # ZMQ
  gvpp
  tinyprocess
  nanolog
  reactcpp
  ${PcapPlusPlus}
  pybind11::embed
  cefdll_wrapper
  serial
  rtaudio
  Gist)

# Common Fuzzer libs
set(MINIMAL_FUZZER_LIBS
  ${CMAKE_THREAD_LIBS_INIT}
  wdissector_static
  fmt::fmt
  imgui
  Folly::folly
  pagmo
  shm_interface
  gvc # From graphviz
  cgraph # From graphviz
  libzmq # ZMQ
  gvpp
  tinyprocess
  nanolog
  reactcpp
  ${PcapPlusPlus}
  pybind11::embed
  serial
  rtaudio
  Gist)

# Common Fuzzer libs
set(EXPLOITER_LIBS
  ${CMAKE_THREAD_LIBS_INIT}
  wdissector_static
  fmt::fmt
  imgui
  Folly::folly
  libzmq # ZMQ
  gvpp
  tinyprocess
  nanolog
  reactcpp
  ${PcapPlusPlus}
  serial
  rtaudio
  Gist)

# ------- Fuzzers -------

# 5GFuzzer executable
set(5G_FUZZER_SRC src/5g_fuzzer.cpp libs/profiling.c)
add_executable(5g_fuzzer ${5G_FUZZER_SRC})
target_link_libraries(5g_fuzzer PRIVATE ${FUZZER_LIBS})
target_compile_options(5g_fuzzer PRIVATE -w -O3 -g3 -fno-eliminate-unused-debug-symbols)
target_compile_definitions(5g_fuzzer PRIVATE -DFUZZ_5G)
target_precompile_headers(5g_fuzzer PRIVATE src/PCH5G.hpp)

# SHM Test
add_executable(shm_server libs/shm_server.c)
add_executable(shm_client libs/shm_client.c)
target_link_libraries(shm_server pthread shm_interface -lrt)
target_link_libraries(shm_client pthread shm_interface -lrt)

# WDMapper
add_executable(wdmapper src/WDMapper.cpp)
target_link_libraries(wdmapper PRIVATE wdissector_static gvc cgraph gvpp
  ${PcapPlusPlus} reactcpp fmt::fmt Boost::iostreams Folly::folly)
target_compile_options(wdmapper PRIVATE -w -O0)
target_precompile_headers(wdmapper PRIVATE src/WDMapper.h)

# ---------------- Custom Commands --------------------
# Generate Global config class (GlobalConfig.hpp)
add_custom_target(
  quicktype_gen_config
  COMMAND
  quicktype
  ../configs/global_config.json
  -l cpp -o GlobalConfig.hpp
  --no-boost --code-format with-struct
  --hide-null-optional
  --top-level GlobalConfig &&
  ../scripts/gen_default_config.sh
  default_config
  ../configs/global_config.json
  GlobalConfig.hpp >> GlobalConfig.hpp &&
  ../scripts/fix_global_config.sh GlobalConfig.hpp
  COMMENT "Generating GlobalConfig.hpp from global_config.json"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/)

if(SWIG_EXIST)
  # Generate GO bindings
  set(INSTALL_GO_PACKAGE TRUE)
  add_custom_target(
    bindings_go_gen
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/bindings/go/wdissector/
    COMMAND
    cp ${PROJECT_SOURCE_DIR}/src/wdissector.h wdissector.h && swig -go -cgo
    -intgosize 64 -I./libs/wireshark/ `pkg-config --cflags-only-I glib-2.0` wdissector.i
    && sed -i "'/import \"C\"/i\/\/ #cgo CFLAGS: -Ilibs/wireshark/ -Ilibs/wireshark/include'" wdissector.go
    && sed -i "'/import \"C\"/i\/\/ #cgo pkg-config: glib-2.0'" wdissector.go
    && sed -i "'/import \"C\"/i\/\/ #cgo LDFLAGS: -L./bin -Wl,-rpath=\$$ORIGIN/bin -lwdissector'"
    wdissector.go
    && ln -sfn ../../bin/ ${PROJECT_SOURCE_DIR}/bindings/go/bin
    && ln -sfn ../../libs/ ${PROJECT_SOURCE_DIR}/bindings/go/libs
    BYPRODUCTS ${PROJECT_SOURCE_DIR}/bindings/go/wdissector/wdissector.h
    ${PROJECT_SOURCE_DIR}/bindings/go/wdissector/wdissector.go
    ${PROJECT_SOURCE_DIR}/bindings/go/wdissector/wdissector_wrap.c
    COMMENT "Generating GO bindings (bindings/go/wdissector)")

  # Generate Python bindings
  add_custom_target(
    bindings_python_gen
    COMMAND
    touch bindings/python/wdissector.i && ${CMAKE_MAKE_PROGRAM} -C build bindings_python
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

  add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/bindings/python/wdissector_wrap.cxx
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDS
    bindings/python/wdissector.i
    COMMAND
    echo "-------------------SWIG-----------------------"
    && swig -lbin -c++ -python bindings/python/wdissector.i || true && sed -i '1i import sys\;sys.path.insert(0, \"bin\") ' bindings/python/wdissector.py
    && echo "-------------------SWIG-----------------------"
    COMMENT "Generating Python bindings (bindings/python/wdissector_wrap.cxx)")

  add_library(bindings_python SHARED ${PROJECT_SOURCE_DIR}/bindings/python/wdissector_wrap.cxx)
  target_link_libraries(bindings_python PRIVATE ${MINIMAL_FUZZER_LIBS})
  target_compile_options(bindings_python PRIVATE -w -O0 -g3 -fpic -fno-eliminate-unused-debug-symbols)
  set_target_properties(bindings_python PROPERTIES CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN 0)
  set_target_properties(bindings_python PROPERTIES OUTPUT_NAME "_wdissector")
  set_target_properties(bindings_python PROPERTIES PREFIX "")
endif()

# ---------------- INSTALL Config --------------------
log(INFO "Build RPath: ${CMAKE_BUILD_RPATH}")

export(
  TARGETS wdissector wireapi
  NAMESPACE wdissector::
  FILE ${PROJECT_BINARY_DIR}/wdissector-targets.cmake)
