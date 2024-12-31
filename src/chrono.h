#pragma once
#include <stdint.h>

typedef int64_t microseconds_t;
typedef int64_t milliseconds_t;

microseconds_t get_current_microseconds();
void sleep_microseconds(microseconds_t time);

inline milliseconds_t get_milliseconds(microseconds_t microseconds) {
    return microseconds / 1000l;
}