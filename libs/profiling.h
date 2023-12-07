#pragma once
#ifndef __PROFILING
#define __PROFILING

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void profiling_timer_start(uint8_t timer_id);

long profiling_timer_end(uint8_t timer_id);

void profiling_timer_end_print(const char *text, uint8_t timer_id, long count);

#define PROFILING_MAX_TIMERS 32
#define PROFILING_LOOP_NUMBER 1000

#define EMPTY_FUNC(...) \
    do {                \
    } while (0)

#define PROFILING_START profiling_timer_start
#define PROFILING_END profiling_timer_end
#define PROFILING_END_PRINT profiling_timer_end_print

#define PROFILING_LOOP_START  \
    profiling_timer_start(0); \
    for (uint32_t i = 0; i < PROFILING_LOOP_NUMBER; i++) {

#define PROFILING_LOOP_STOP(name) \
    }                             \
    ;                             \
    profiling_timer_end_print(name, 0, PROFILING_LOOP_NUMBER);



#ifdef __cplusplus
}
#endif
#endif
