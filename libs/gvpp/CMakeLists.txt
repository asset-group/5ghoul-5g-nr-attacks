cmake_minimum_required(VERSION 3.5)
project(gvpp)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "./lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "./lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "./bin")

add_library(gvpp STATIC gvpp.hpp gvpp.cpp)

export(TARGETS gvpp FILE ${PROJECT_BINARY_DIR}/gvpp-targets.cmake)
