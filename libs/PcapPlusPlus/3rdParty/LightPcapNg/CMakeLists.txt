cmake_minimum_required (VERSION 3.0)
project (light_pcapng C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library (light_pcapng STATIC
	LightPcapNg/src/light_io.c
	LightPcapNg/src/light_pcapng.c
	LightPcapNg/src/light_pcapng_cont.c
	LightPcapNg/src/light_platform.c
	LightPcapNg/src/light_compression.c
	LightPcapNg/src/light_null_compression.c
	LightPcapNg/src/light_manipulate.c
	LightPcapNg/src/light_internal.c
	LightPcapNg/src/light_alloc.c
	LightPcapNg/src/light_advanced.c
	LightPcapNg/src/light_option.c
	LightPcapNg/src/light_pcapng_ext.c
	LightPcapNg/include/light_debug.h
	LightPcapNg/include/light_internal.h
	LightPcapNg/include/light_pcapng_ext.h
	LightPcapNg/include/light_pcapng.h
	LightPcapNg/include/light_platform.h
	LightPcapNg/include/light_special.h
	LightPcapNg/include/light_types.h
	LightPcapNg/include/light_util.h
 )

add_library (light_pcapng_static STATIC
	LightPcapNg/src/light_io.c
	LightPcapNg/src/light_pcapng.c
	LightPcapNg/src/light_pcapng_cont.c
	LightPcapNg/src/light_platform.c
	LightPcapNg/src/light_compression.c
	LightPcapNg/src/light_null_compression.c
	LightPcapNg/src/light_manipulate.c
	LightPcapNg/src/light_internal.c
	LightPcapNg/src/light_alloc.c
	LightPcapNg/src/light_advanced.c
	LightPcapNg/src/light_option.c
	LightPcapNg/src/light_pcapng_ext.c
	LightPcapNg/include/light_debug.h
	LightPcapNg/include/light_internal.h
	LightPcapNg/include/light_pcapng_ext.h
	LightPcapNg/include/light_pcapng.h
	LightPcapNg/include/light_platform.h
	LightPcapNg/include/light_special.h
	LightPcapNg/include/light_types.h
	LightPcapNg/include/light_util.h
 )


target_compile_definitions(light_pcapng
	PUBLIC -DUNIVERSAL
)

target_include_directories(light_pcapng PUBLIC
	${PROJECT_SOURCE_DIR}/LightPcapNg/include/
)

target_compile_definitions(light_pcapng_static
	PUBLIC -DUNIVERSAL
)

target_include_directories(light_pcapng_static PUBLIC
	${PROJECT_SOURCE_DIR}/LightPcapNg/include/
)
