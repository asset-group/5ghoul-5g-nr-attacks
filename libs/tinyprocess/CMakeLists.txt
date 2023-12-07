cmake_minimum_required(VERSION 3.17)

project(tinyprocess)

if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-std=c++11 -Wall -Wextra)
endif()

add_library(tinyprocess process.cpp)

if(WIN32)
  target_sources(tinyprocess PRIVATE process_win.cpp)
  # If compiled using MSYS2, use sh to run commands
  if(MSYS)
    target_compile_definitions(tinyprocess PUBLIC MSYS_PROCESS_USE_SH)
  endif()
else()
  target_sources(tinyprocess PRIVATE process_unix.cpp)
endif()

find_package(Threads REQUIRED)

target_link_libraries(tinyprocess ${CMAKE_THREAD_LIBS_INIT} util)
target_include_directories(tinyprocess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
