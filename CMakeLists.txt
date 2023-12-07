cmake_minimum_required(VERSION 3.18.4)
set(CMAKE_C_COMPILER "clang-15")
set(CMAKE_CXX_COMPILER "clang++-15")

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
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${COMMON_FLAGS})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${COMMON_FLAGS})
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_BUILD_RPATH $ORIGIN bin modules/python/install/lib/)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")

# Configure linker (enforce lld-15)
add_link_options("-fuse-ld=lld-15")

# Print some options
log(INFO "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
log(INFO "EXPORT_COMPILE_COMMANDS: ${EXPORT_COMPILE_COMMANDS}")

# ---------------- Includes --------------------
# Global Includes
include_directories(
  AFTER ${PROJECT_SOURCE_DIR}/ ${PROJECT_SOURCE_DIR}/src/
  ${PROJECT_SOURCE_DIR}/libs/ ${PROJECT_SOURCE_DIR}/libs/libpcap/)

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

# libPCAP
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/libpcap)
set(HAVE_LIBPCAP 1)
set(ENABLE_PCAP ON)
set(CAN_SET_CAPTURE_BUFFER_SIZE 1)
set(PCAP_FOUND ON)

# GLFW3 (No Wayland)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_INSTALL OFF)
set(GLFW_USE_OSMESA OFF)
set(GLFW_USE_WAYLAND OFF)
set(BUILD_SHARED_LIBS ON)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/glfw)

# Configure Pagmo2
log(INFO "Building libpagmo.so")

# 1) Download and preapre liboost headers
set(LIBBOOST_DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/libs/libboost/)
set(LIBBOOST_DOWNLOAD_URL https://www.mirrorservice.org/sites/tinycorelinux.net/13.x/x86_64/tcz/src/boost/boost_1_78_0.tar.gz)
string(REGEX MATCH "boost_[^.]*" LIBBOOST_VERSION "${LIBBOOST_DOWNLOAD_URL}")
set(BOOST_ROOT ${LIBBOOST_DOWNLOAD_DIR}/${LIBBOOST_VERSION}/)
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_NO_WARN_NEW_VERSIONS 1)

if(NOT EXISTS ${LIBBOOST_DOWNLOAD_DIR}/boost)
  message(STATUS "Downloading libboost")
  file(
    DOWNLOAD
    ${LIBBOOST_DOWNLOAD_URL}
    ${PROJECT_SOURCE_DIR}/libs/libboost/boost.tar.gz
    SHOW_PROGRESS)

  # Extract libboost
  execute_process(
    COMMAND bash -c "tar -xf boost.tar.gz && ln -sfn ${LIBBOOST_VERSION}/boost boost && sync && rm boost.tar.gz"
    WORKING_DIRECTORY ${LIBBOOST_DOWNLOAD_DIR})

  # Compile libboost required modules
  execute_process(
    COMMAND bash -c "sync && ./bootstrap.sh && sync && ./b2 --with-serialization --with-iostreams --with-context --with-filesystem --with-program_options --with-regex --with-system --with-thread"
    WORKING_DIRECTORY ${LIBBOOST_DOWNLOAD_DIR}/${LIBBOOST_VERSION}/)
endif()

if(NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libboost_serialization.so)
  # Copy boost binaries to bin folder
  execute_process(
    COMMAND bash -c "mkdir -p ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} && sync && find ./stage/lib -name '*.so*' -exec cp -prv '{}' '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}' ';'"
    WORKING_DIRECTORY ${LIBBOOST_DOWNLOAD_DIR}/${LIBBOOST_VERSION}/)
endif()

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

# 4) Add subdirectories and link libboost to pagmo
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/pagmo2)
target_link_libraries(pagmo PUBLIC Boost::iostreams)

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

# RTAudio
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/rtaudio/)

# PyBind11 (Run Python Modules from C++)
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/modules/python/success)
  message(STATUS "Downloading Python 3.8.16")
  file(
    DOWNLOAD
    https://github.com/indygreg/python-build-standalone/releases/download/20230116/cpython-3.8.16+20230116-${CMAKE_HOST_SYSTEM_PROCESSOR}-unknown-linux-gnu-lto-full.tar.zst
    ${PROJECT_SOURCE_DIR}/modules/python.tar.zst
    SHOW_PROGRESS)
  execute_process(
    COMMAND bash -c "sync && tar -I zstd -xf python.tar.zst || true"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules/
    ERROR_VARIABLE PYTHON_ERROR)

  execute_process(
    COMMAND bash -c "rm python.tar.zst || true && sync"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules/)

  # Create symbolic links to python3 libs
  execute_process(
    COMMAND bash -c "ln -sfn ../modules/python/install/lib/libpython3* ."
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  )

  # Install requirements from modules/server/requirements.txt
  set(ENV{PIP_DISABLE_PIP_VERSION_CHECK} 1)
  execute_process(
    COMMAND bash -c
    "./python/install/bin/python3 -m pip install -r ./requirements.txt --use-pep517 --root-user-action=ignore"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules/
    ERROR_VARIABLE PYTHON_ERROR)

  if(PYTHON_ERROR)
    message(FATAL_ERROR ${PYTHON_ERROR})
  endif(PYTHON_ERROR)

  # Save success indication file
  execute_process(
    COMMAND bash -c "touch python/success"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules/
    ERROR_VARIABLE PYTHON_ERROR)

  if(PYTHON_ERROR)
    message(FATAL_ERROR ${PYTHON_ERROR})
  endif(PYTHON_ERROR)
endif()

set(PYBIND11_PYTHONLIBS_OVERWRITE OFF)
set(PYTHON_EXECUTABLE ${PROJECT_SOURCE_DIR}/modules/python/install/bin/python3)
set(PYTHON_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/modules/python/install/include/python3.8/)
set(PYTHON_LIBDIR ${PROJECT_SOURCE_DIR}/modules/python/install/lib/)
set(PYTHON_LIBRARY ${PROJECT_SOURCE_DIR}/modules/python/install/lib/libpython3.8.so)

add_subdirectory(${PROJECT_SOURCE_DIR}/libs/pybind11/)
target_link_directories(pybind11::embed INTERFACE
  ${PROJECT_SOURCE_DIR}/modules/python/install/lib/)

# Dynamo (shared library loader)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/dynalo/)

# Import and build libzmq
add_subdirectory(libs/libzmq)

# Import double-conversion (folly dependency)
set(DOUBLE_CONVERSION_LIBRARY double-conversion)
set(DOUBLE_CONVERSION_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs/double-conversion/double-conversion/)
add_subdirectory(libs/double-conversion)

# Import libfmt (static build)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(libs/fmt)
set(BUILD_SHARED_LIBS ON)

# Import reduced libfolly 0.57.0 (shared build)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(libs/folly)
target_include_directories(folly PUBLIC libs/folly/folly)
set(BUILD_SHARED_LIBS ON)

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
  ${PROJECT_SOURCE_DIR}/ ${PROJECT_SOURCE_DIR}/libs libs/glfw/include)
target_link_libraries(imgui PRIVATE GL glfw rt m dl ${FREETYPE_LIBRARIES})

add_library(nanolog STATIC libs/NanoLog.cpp)
target_include_directories(nanolog PRIVATE ${PROJECT_SOURCE_DIR}/)

# Add libmpsse-static (FTDI MPSSI - SPI mode)
add_library(mpsse-static STATIC IMPORTED)
set_target_properties(
  mpsse-static PROPERTIES IMPORTED_LOCATION
  ${PROJECT_SOURCE_DIR}/libs/libmpsse/libmpsse-static.a)
target_link_libraries(mpsse-static INTERFACE pthread udev)
target_include_directories(mpsse-static INTERFACE libs/libmpsse/)

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
  fmt
  pagmo
  imgui
  folly
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
  rtaudio)

# Common Fuzzer libs
set(MINIMAL_FUZZER_LIBS
  ${CMAKE_THREAD_LIBS_INIT}
  wdissector_static
  fmt
  imgui
  folly
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
  rtaudio)

# Common Fuzzer libs
set(EXPLOITER_LIBS
  ${CMAKE_THREAD_LIBS_INIT}
  wdissector_static
  fmt
  imgui
  folly
  libzmq # ZMQ
  gvpp
  tinyprocess
  nanolog
  reactcpp
  ${PcapPlusPlus}
  serial
  rtaudio)

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
  ${PcapPlusPlus} reactcpp fmt Boost::iostreams folly)
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
