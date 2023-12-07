# CEF-CMake 1.0.0
# Copyright (c) 2019 Borislav Stanimirov
#
# Distributed under the MIT Software License
# See accompanying file LICENSE.txt or copy at
# http://opensource.org/licenses/MIT
#
# Configuration file for CEF-CMake
# Include in your root CMakeLists.txt
# (or, stricly speaking, the root CMakeLists to all CEF-related projects)
set(CEF_CMAKE_INCLUDED YES)

option(CEF_USE_SANDBOX "CEF-CMake: Enable or disable use of the CEF sandbox." ON)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(CEF_CMAKE_OS_MACOSX 1)
    set(CEF_CMAKE_OS_POSIX 1)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(CEF_CMAKE_OS_LINUX 1)
    set(CEF_CMAKE_OS_POSIX 1)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(CEF_CMAKE_OS_WINDOWS 1)
else()
    message(FATAL_ERROR "CEF-CMake: Unsupported target platform")
endif()

# Set a default build type if none was specified
# MSVC has no Default build type anyway, so skip this
if(NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    message(WARNING "CEF-CMake: 'Default' build type is not supported by CEF-CMake. Setting build type to 'Release'.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(MSVC AND CEF_USE_SANDBOX)
    message(STATUS "CEF-CMake: Replacing /MD->/MT in C and CXX_FLAGS. Required by CEF sandbox.")
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
    )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

if(CEF_CMAKE_OS_WINDOWS)
    set(CEF_CMAKE_EXECUTABLE_RESOURCES
        ${CMAKE_CURRENT_LIST_DIR}/../res/win/cef.exe.manifest
        ${CMAKE_CURRENT_LIST_DIR}/../res/win/compatibility.manifest
    )
elseif(CEF_CMAKE_OS_MACOSX)
    message(FATAL_ERROR "CEF-CMake: Executable resources for platform not supported yet")
else()
    set(CEF_CMAKE_EXECUTABLE_RESOURCES)
endif()
