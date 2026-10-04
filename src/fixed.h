/* Q16.16 arithmetic. Intermediate products use 64 bits; public results retain
 * the signed word rounding and two's-complement wrap used by compatibility data. */
#ifndef FIXED_H
#define FIXED_H
#include <stdint.h>
#include <math.h>

static inline int32_t fx_mul(int32_t a, int32_t b)
{
    int64_t product = (int64_t)a * b;
    int64_t scaled = product / 65536;
    if (product < 0 && product % 65536) scaled--;
    return (int32_t)scaled;
}
static inline int32_t fx_div(int32_t a, int32_t b)
{
    if (!b) return 0;
    int64_t numerator = (int64_t)a * 65536;
    uint64_t magnitude = numerator < 0 ? -numerator : numerator;
    uint64_t divisor = b < 0 ? -(int64_t)b : b;
    uint32_t rounded = (uint32_t)((magnitude + divisor / 2) / divisor);
    uint32_t bits = (a < 0) != (b < 0) ? 0u - rounded : rounded;
    return (int32_t)bits;
}
static inline int fx_round(int32_t value)
{
    int64_t biased = (int64_t)value + 32768;
    int64_t integer = biased / 65536;
    if (biased < 0 && biased % 65536) integer--;
    return (int16_t)integer;
}
static inline int32_t fx_from_word(int16_t value) { return (int32_t)value * 65536; }
static inline int32_t real_to_fx(double value) { return (int32_t)floor(value * 65536.0 + 0.5); }
#endif
