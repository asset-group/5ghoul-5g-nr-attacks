#pragma once
#include <time.h>
#include <stdio.h>
#include "profiling.h"

static struct timespec start_time_array[PROFILING_MAX_TIMERS];

// call this function to start a nanosecond-resolution timer
void profiling_timer_start(uint8_t timer_id)
{
    clock_gettime(CLOCK_MONOTONIC, &start_time_array[timer_id]);
}

// call this function to end a timer, returning nanoseconds elapsed as a long
long profiling_timer_end(uint8_t timer_id)
{
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long diffInNanos = (end_time.tv_sec - start_time_array[timer_id].tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time_array[timer_id].tv_nsec);
    return diffInNanos;
}

void profiling_timer_end_print(const char *text, uint8_t timer_id, long count)
{
    long time_elapsed_nanos = profiling_timer_end(timer_id);
    if (count == 0)
        count = 1;
    printf("[%s] Time (us): %.2f\n", text, (double)time_elapsed_nanos / (double)count / 1000.0);
}