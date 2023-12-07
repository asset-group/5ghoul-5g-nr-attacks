cmake_minimum_required(VERSION 3.17)
project(serial)

if(APPLE)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(FOUNDATION_LIBRARY Foundation)
endif()

if(UNIX AND NOT APPLE)
  # If Linux, add rt and pthread
  set(rt_LIBRARIES rt)
  set(pthread_LIBRARIES pthread)
else()
  # Otherwise normal call
endif()

# Sources
set(serial_SRCS src/serial.cc include/serial/serial.h include/serial/v8stdint.h)
if(APPLE)
  # If OSX
  list(APPEND serial_SRCS src/impl/unix.cc)
  list(APPEND serial_SRCS src/impl/list_ports/list_ports_osx.cc)
elseif(UNIX)
  # If unix
  list(APPEND serial_SRCS src/impl/unix.cc)
  list(APPEND serial_SRCS src/impl/list_ports/list_ports_linux.cc)
else()
  # If windows
  list(APPEND serial_SRCS src/impl/win.cc)
  list(APPEND serial_SRCS src/impl/list_ports/list_ports_win.cc)
endif()

# Add serial library
add_library(${PROJECT_NAME} ${serial_SRCS})
if(APPLE)
  target_link_libraries(${PROJECT_NAME} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif(UNIX)
  target_link_libraries(${PROJECT_NAME} rt pthread)
else()
  target_link_libraries(${PROJECT_NAME} setupapi)
endif()

# Uncomment for example
# add_executable(serial_example examples/serial_example.cc)
# add_dependencies(serial_example ${PROJECT_NAME})
# target_link_libraries(serial_example ${PROJECT_NAME})

# Include headers
include_directories(include)

# Install executable
install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION})

export(TARGETS ${PROJECT_NAME}
       FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake)
