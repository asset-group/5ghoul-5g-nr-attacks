# CEF-CMake 1.0.0 Copyright (c) 2019 Borislav Stanimirov
#
# Distributed under the MIT Software License See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT
#
if(NOT CEF_CMAKE_INCLUDED)
  message(
    FATAL_ERROR
      "CEF-CMake: Configuration not included. You need to include `cef_cmake` in your root CMakeLists.txt file"
  )
endif()

if(NOT CEF_VERSION)
  # set(CEF_VERSION 73.1.3+g46cf800+chromium-73.0.3683.75) set(CEF_VERSION
  # 73.1.12+gee4b49f+chromium-73.0.3683.75) set(CEF_VERSION
  # 74.1.13+g98f22d3+chromium-74.0.3729.108) set(CEF_VERSION
  # 74.1.19+gb62bacf+chromium-74.0.3729.157) 
  if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set(CEF_VERSION 75.0.3+g699a922+chromium-75.0.3770.80)
  else()
    set(CEF_VERSION 79.1.38+gecefb59+chromium-79.0.3945.130)
  endif()
  # set(CEF_VERSION 76.1.5+gd8a577c+chromium-76.0.3809.87)
  # set(CEF_VERSION 79.1.3+ga5342ed+chromium-79.0.3945.88) set(CEF_VERSION
  # 83.3.9+ged43e55+chromium-83.0.4103.61) set(CEF_VERSION
  # 86.0.21+g6a2c8e7+chromium-86.0.4240.183)

  message(
    STATUS "CEF-CMake: CEF_VERSION not specified. Defaulting to ${CEF_VERSION}")
endif()

if(NOT CEF_CMAKE_OUTPUT_DIR)
  if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    message(
      SEND_ERROR
        "CEF-CMake: Neither CEF_CMAKE_OUTPUT_DIR nor CMAKE_RUNTIME_OUTPUT_DIRECTORY was specified. You need to specify one for the binary assets of CEF to be copied."
    )
  endif()
  # CEF_CMAKE_OUTPUT_DIR is used to copy the required shared libraries next to
  # the executable
  set(CEF_CMAKE_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  message(
    STATUS
      "CEF-CMake: CEF_CMAKE_OUTPUT_DIR was not specified. Defaulting to CMAKE_RUNTIME_OUTPUT_DIRECTORY: ${CEF_CMAKE_OUTPUT_DIR}"
  )
endif()

if(MSVC)
  set(CEF_CMAKE_OUTPUT_DIR ${CEF_CMAKE_OUTPUT_DIR}/Release)
endif()

if(CEF_CMAKE_OS_LINUX)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
      set(cefName cef_binary_${CEF_VERSION}_linux64_minimal)
    else()
      # ARM64 version
      set(cefName cef_binary_${CEF_VERSION}_linuxarm64_minimal)
    endif()
  else()
    set(cefName cef_binary_${CEF_VERSION}_linux32_minimal)
  endif()
elseif(CEF_CMAKE_OS_WINDOWS)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(cefName cef_binary_${CEF_VERSION}_windows64_minimal)
  else()
    set(cefName cef_binary_${CEF_VERSION}_windows32_minimal)
  endif()
else()
  message(FATAL_ERROR "CEF-CMake: Download platform not supported yet")
endif()

set(cefArchiveURL http://cef-builds.spotifycdn.com/${cefName}.tar.bz2)
# fix the url as the version may contain pluses
string(REGEX REPLACE "\\+" "%2B" cefArchiveURL ${cefArchiveURL})
set(cefArchive ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}.tar.bz2)

if(NOT EXISTS ${cefArchive})
  # download cef version
  message(STATUS "CEF-CMake: Downloading CEF ${cefArchiveURL}")
  file(DOWNLOAD ${cefArchiveURL} ${cefArchive} SHOW_PROGRESS)

  # ... and extract
  message(STATUS "CEF-CMake: Extracting ${cefArchive}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${cefArchive}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${cefName})
  execute_process(COMMAND ${CMAKE_COMMAND} -E rm ${cefArchive}
              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  message(FATAL_ERROR "Could not download CEF, check URL: ${cefArchiveURL}")
endif()

set(srcs)
set(wdir ${cefName}/libcef_dll)
file(GLOB_RECURSE srcs "${wdir}/*.cc" "${wdir}/*.mm" "${wdir}/*.h"
     "${cefName}/include/*.h")

add_library(cefdll_wrapper SHARED ${srcs})

target_compile_definitions(
  cefdll_wrapper
  PRIVATE # Creating the CEF wrapper library. Do not define this for dependent
          # targets.
          -DWRAPPING_CEF_SHARED
  PUBLIC # Allow C++ programs to use stdint.h macros specified in the C99
         # standard that aren't in the C++ standard (e.g. UINT8_MAX, INT64_MIN,
         # etc)
         -D__STDC_CONSTANT_MACROS
         -D__STDC_FORMAT_MACROS)

target_compile_options(cefdll_wrapper PRIVATE -w)

target_include_directories(cefdll_wrapper PUBLIC ${cefName})

add_custom_command(
  TARGET cefdll_wrapper
  POST_BUILD
  COMMENT "cefdll_wrapper: Copying CEF resources"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CEF_CMAKE_OUTPUT_DIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Resources/locales
    ${CEF_CMAKE_OUTPUT_DIR}/locales
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Resources/cef.pak
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Resources/cef_100_percent.pak
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Resources/cef_200_percent.pak
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Resources/cef_extensions.pak
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Resources/devtools_resources.pak
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Resources/icudtl.dat
    ${CEF_CMAKE_OUTPUT_DIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Resources/icudtl.dat
    ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/)

if(CEF_CMAKE_OS_LINUX)

  log(INFO "Found dev libcef.so")
  log(STATUS "${LIBCEF_SO}")
  target_link_directories(cefdll_wrapper PUBLIC
                          ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/)

  target_link_libraries(cefdll_wrapper INTERFACE X11 pthread optimized cef)

  add_custom_command(
    TARGET cefdll_wrapper
    POST_BUILD
    COMMENT "cefdll_wrapper: Copying CEF binaries and includes"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CEF_CMAKE_OUTPUT_DIR}
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/swiftshader
      ${CEF_CMAKE_OUTPUT_DIR}/swiftshader
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/include
      ${CEF_CMAKE_OUTPUT_DIR}/cef/include
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/chrome-sandbox
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/libcef.so
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/libEGL.so
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/libGLESv2.so
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/natives_blob.bin
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/snapshot_blob.bin
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/v8_context_snapshot.bin
      ${CEF_CMAKE_OUTPUT_DIR})

  install(
    TARGETS cefdll_wrapper
    EXPORT CEFTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  if(CEF_USE_SANDBOX)
    target_compile_definitions(cefdll_wrapper PUBLIC -DCEF_USE_SANDBOX)
  endif()

elseif(CEF_CMAKE_OS_WINDOWS)

  target_compile_definitions(
    cefdll_wrapper PUBLIC -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE)

  target_link_libraries(
    cefdll_wrapper
    PUBLIC comctl32.lib rpcrt4.lib shlwapi.lib ws2_32.lib optimized
           ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/libcef.lib)

  add_custom_command(
    TARGET cefdll_wrapper
    POST_BUILD
    COMMENT "cefdll_wrapper: Copying CEF binaries and includes"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CEF_CMAKE_OUTPUT_DIR}
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/swiftshader
      ${CEF_CMAKE_OUTPUT_DIR}/swiftshader
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/include
      ${CEF_CMAKE_OUTPUT_DIR}/cef/include
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/chrome_elf.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/d3dcompiler_43.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/d3dcompiler_47.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/libcef.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/libEGL.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/libGLESv2.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/natives_blob.bin
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/snapshot_blob.bin
      ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/v8_context_snapshot.bin
      ${CEF_CMAKE_OUTPUT_DIR})

  if(CEF_USE_SANDBOX)
    target_compile_definitions(cefdll_wrapper PUBLIC -DCEF_USE_SANDBOX
                                                     -DPSAPI_VERSION=1)

    target_link_libraries(
      cefdll_wrapper
      PUBLIC dbghelp.lib
             psapi.lib
             version.lib
             wbemuuid.lib
             winmm.lib
             optimized
             ${CMAKE_CURRENT_SOURCE_DIR}/${cefName}/Release/cef_sandbox.lib)
  endif()

else()
  message(FATAL_ERROR "CEF-CMake: Build platform not supported yet")
endif()
