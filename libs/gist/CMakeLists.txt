cmake_minimum_required (VERSION 3.12)
project (Gist VERSION 1.0.6)

set (CMAKE_CXX_STANDARD 11)

option (BUILD_TESTS "Build tests" OFF)

add_subdirectory (src)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory (tests)
endif (BUILD_TESTS)

set (CMAKE_SUPPRESS_REGENERATION true)

