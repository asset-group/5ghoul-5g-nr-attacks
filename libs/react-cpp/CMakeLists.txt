project(reactcpp)
cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libev)

file(GLOB_RECURSE REACTCPP_SOURCE "src/*.cpp")

add_library(reactcpp STATIC ${REACTCPP_SOURCE})
target_include_directories(reactcpp PRIVATE ./ include src libev)
target_compile_options(reactcpp PRIVATE -Wall -MMD -w -Wno-unused-function)
target_link_libraries(reactcpp ev_static)
