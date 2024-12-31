#pragma once
#include <stdint.h>

#define NUM_ELEMS(ARRAY) (sizeof((ARRAY))/sizeof((ARRAY)[0]))

#define NULL_UINT8 ((uint8_t)-1)
#define NULL_UINT16 ((uint16_t)-1)
#define NULL_UINT32 ((uint32_t)-1)
#define NULL_UINT64 ((uint64_t)-1)

inline uint32_t clamp_uint32(uint32_t value, uint32_t min, uint32_t max) {
    if (value < min) { return min; }
    if (value > max) { return max; }
    return value;
}

inline uint32_t max_uint32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

inline uint32_t div_ceil_uint32(uint32_t a, uint32_t b) {
    return (a + b - 1u) / b;
}

inline uint32_t ceil_to_next_multiple(uint32_t value, uint32_t step) {
    uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
    return step * divide_and_ceil;
}