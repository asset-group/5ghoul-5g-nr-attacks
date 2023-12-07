cmake_minimum_required(VERSION 2.8.12)
project(libev C)
set(LIBEV_VERSION 4.33)
set(LIBEV_VERSION_CURRENT 4)
set(LIBEV_VERSION_AGE 0)
set(LIBEV_VERSION_REVISION 0)

include(${PROJECT_SOURCE_DIR}/CMakeMacroLibtoolFile.cmake)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckCSourceCompiles)

if(NOT DEFINED BUILD_SHARED_LIBS)
    option(BUILD_SHARED_LIBS "Build libev as a shared library" ON)
endif()
if(NOT DEFINED BUILD_STATIC_LIBS)
    option(BUILD_STATIC_LIBS "Build libev as a static library" ON)
endif()
if(NOT DEFINED BUILD_PIC_STATIC_LIBS AND NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    option(BUILD_PIC_STATIC_LIBS "Build the libev static library position independent" ON)
endif()

check_include_file(sys/inotify.h  HAVE_SYS_INOTIFY_H)
check_include_file(sys/epoll.h    HAVE_SYS_EPOLL_H)
check_include_file(sys/event.h    HAVE_SYS_EVENT_H)
check_include_file(port.h         HAVE_PORT_H)
check_include_file(poll.h         HAVE_POLL_H)
check_include_file(sys/select.h   HAVE_SYS_SELECT_H)
check_include_file(sys/eventfd.h  HAVE_SYS_EVENTFD_H)
check_include_file(sys/signalfd.h HAVE_SYS_SIGNALFD_H)

if(HAVE_SYS_INOTIFY_H)
    check_symbol_exists(inotify_init sys/inotify.h  HAVE_INOTIFY_INIT)
endif()
if(HAVE_SYS_EPOLL_H)
    check_symbol_exists(epoll_ctl    sys/epoll.h    HAVE_EPOLL_CTL)
endif()
if(HAVE_SYS_EVENT_H)
    check_symbol_exists(kqueue       sys/event.h    HAVE_KQUEUE)
endif()
if(HAVE_PORT_H)
    check_symbol_exists(port_create  port.h         HAVE_PORT_CREATE)
endif()
if(HAVE_POLL_H)
    check_symbol_exists(poll         poll.h         HAVE_POLL)
endif()
if(HAVE_SYS_SELECT_H)
    check_symbol_exists(select       sys/select.h   HAVE_SELECT)
endif()
if(HAVE_SYS_EVENTFD_H)
    check_symbol_exists(eventfd      sys/eventfd.h  HAVE_EVENTFD)
endif()
if(HAVE_SYS_SIGNALFD_H)
    check_symbol_exists(signalfd     sys/signalfd.h HAVE_SIGNALFD)
endif()

check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)
if(NOT HAVE_CLOCK_GETTIME)
    if(CMAKE_SYSTEM_NAME STREQUAL Linux)
        message(STATUS "Checking for clock_gettime syscall")
        check_c_source_compiles("\
            #include <unistd.h>\n\
            #include <sys/syscall.h>\n\
            #include <time.h>\n\
            int main(int argc, char *argv[])\n\
            {\n\
                (void) argc;\n\
                (void) argv;\n\
                struct timespec ts;\n\
                int status = syscall (SYS_clock_gettime, CLOCK_REALTIME, &ts);\n\
                return status;\n\
            }\
            "
            HAVE_CLOCK_SYSCALL
        )
    endif()

    if(NOT AVOID_LIBRT AND NOT HAVE_CLOCK_SYSCALL)
        set(CMAKE_REQUIRED_LIBRARIES rt)
        check_symbol_exists(clock_gettime time.h _HAVE_CLOCK_GETTIME_2)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(_HAVE_CLOCK_GETTIME_2)
            set(LIBRT_NAME rt CACHE STRING "Name of the realtime library to explicitly link in")
            set(HAVE_CLOCK_GETTIME ${_HAVE_CLOCK_GETTIME_2})
            unset(_HAVE_CLOCK_GETTIME_2)
        endif()
    endif()
endif()

check_symbol_exists(nanosleep time.h HAVE_NANOSLEEP)
if(NOT AVOID_LIBRT AND NOT HAVE_NANOSLEEP)
    set(CMAKE_REQUIRED_LIBRARIES rt)
    check_symbol_exists(nanosleep time.h _HAVE_NANOSLEEP_2)
    unset(CMAKE_REQUIRED_LIBRARIES)
    if(_HAVE_NANOSLEEP_2)
        set(LIBRT_NAME rt CACHE STRING "Name of the realtime library to explicitly link in")
        set(HAVE_NANOSLEEP ${_HAVE_NANOSLEEP_2})
        unset(_HAVE_NANOSLEEP_2)
    endif()
endif()

check_symbol_exists(floor math.h HAVE_FLOOR)
if(NOT AVOID_LIBM AND NOT HAVE_FLOOR)
    set(CMAKE_REQUIRED_LIBRARIES m)
    check_symbol_exists(floor math.h _HAVE_FLOOR_2)
    unset(CMAKE_REQUIRED_LIBRARIES)
    if(_HAVE_FLOOR_2)
        set(LIBM_NAME m CACHE STRING "Name of the math library to explicitly link in")
        set(HAVE_FLOOR ${_HAVE_FLOOR_2})
        unset(_HAVE_FLOOR_2)
    endif()
endif()

configure_file(
    ${PROJECT_SOURCE_DIR}/config.h.cmakein
    ${CMAKE_CURRENT_BINARY_DIR}/evconfig.h
    @ONLY
)

if(NOT DEFINED SKIP_MAN_PAGE)
    if(NOT POD2MAN_COMMAND)
        message(STATUS "Checking for pod2man")
        find_program(
            POD2MAN_COMMAND
            NAMES pod2man
            DOC "POD data to formatted *roff converter"
        )

        if(POD2MAN_COMMAND STREQUAL POD2MAN_COMMAND-NOTFOUND)
            message(STATUS "Checking for pod2man - not found")
            unset(POD2MAN_COMMAND)
        else()
            message(STATUS "Checking for pod2man - found (${POD2MAN_COMMAND})")
        endif()
    endif()

    if(POD2MAN_COMMAND)
        add_custom_command(
            OUTPUT ev.3
            COMMAND ${POD2MAN_COMMAND}
                -n LIBEV
                -r libev-${LIBEV_VERSION}
                -c "libev - high performance full featured event loop"
                -s3 ${PROJECT_SOURCE_DIR}/ev.pod
                >ev.3
            MAIN_DEPENDENCY ev.pod
            COMMENT "Building the libev man page"
        )
        add_custom_target(man ALL DEPENDS ev.3)

        install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/ev.3
            DESTINATION share/man/man3
        )
    endif()
endif()

if(NOT PROJECT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

list(APPEND LIBEV_SRCS
    ev.c
    event.c
)

if(BUILD_STATIC_LIBS)
    add_library(ev_static STATIC ${LIBEV_SRCS})
    target_include_directories(ev_static PUBLIC .)
    set_target_properties(ev_static PROPERTIES
        OUTPUT_NAME ev
        VERSION   ${LIBEV_VERSION_CURRENT}.${LIBEV_VERSION_AGE}.${LIBEV_VERSION_REVISION}
        SOVERSION ${LIBEV_VERSION_CURRENT}
    )
    if(BUILD_PIC_STATIC_LIBS)
        set_target_properties(ev_static PROPERTIES
            POSITION_INDEPENDENT_CODE ON
        )
    endif()

    install(TARGETS ev_static ARCHIVE DESTINATION lib)
endif()

if(BUILD_SHARED_LIBS)
    add_library(ev SHARED ${LIBEV_SRCS})
    target_include_directories(ev PUBLIC .)
    set_target_properties(ev PROPERTIES
        LT_VERSION_CURRENT  ${LIBEV_VERSION_CURRENT}
        LT_VERSION_AGE      ${LIBEV_VERSION_AGE}
        LT_VERSION_REVISION ${LIBEV_VERSION_REVISION}
        VERSION   ${LIBEV_VERSION_CURRENT}.${LIBEV_VERSION_AGE}.${LIBEV_VERSION_REVISION}
        SOVERSION ${LIBEV_VERSION_CURRENT}
    )

    if(LIBRT_NAME OR LIBM_NAME)
        target_link_libraries(ev ${LIBRT_NAME} ${LIBM_NAME})
    endif()

    create_libtool_file(ev_static /lib)

    install(TARGETS ev LIBRARY DESTINATION lib)
endif()

if((BUILD_STATIC_LIBS OR BUILD_SHARED_LIBS) AND NOT SKIP_INSTALL_HEADERS)
    install(
        FILES ev.h ev++.h event.h
        DESTINATION include
    )
endif()
