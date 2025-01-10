#pragma once
#include <stdint.h>

typedef int64_t microseconds_t;
typedef int64_t milliseconds_t;

microseconds_t get_current_microseconds();
void sleep_microseconds(microseconds_t time);

inline microseconds_t get_query_microseconds(uint64_t start, uint64_t end, float timestamp_period) {
    return (microseconds_t) ((float) (end - start) * timestamp_period * 0.001f);
}

inline milliseconds_t get_milliseconds(microseconds_t microseconds) {
    return microseconds / 1000l;
}