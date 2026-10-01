/* 16.16 fixed point helpers matching the original routines in unit 1008 (2830, 2845, 2899, 281a, 27bd). */
#ifndef FIXED_H
#define FIXED_H
#include <stdint.h>
#include <math.h>

/* 1008:2830 - (a*b)>>16 */
static inline int32_t fx_mul(int32_t a, int32_t b) { return (int32_t)(((int64_t)a * b) >> 16); }

/* 1008:2845 - (a<<16)/b rounded half up, sign by operands */
static inline int32_t fx_div(int32_t a, int32_t b)
{
    int neg = 0;
    uint32_t ua = (uint32_t)a, ub = (uint32_t)b;
    if (a < 0) { ua = -ua; neg++; }
    if (b < 0) { ub = -ub; neg--; }
    uint64_t n = (uint64_t)ua << 16;
    uint32_t q = (uint32_t)(n / ub), r = (uint32_t)(n % ub);
    uint32_t half = (ub >> 1) + (ub & 1);
    half -= 1;
    q += half < r;
    return neg ? -(int32_t)q : (int32_t)q;
}

/* 1008:281a - round a 16.16 value to integer */
/* the result is the high word of DX:AX, i.e. a signed 16-bit value */
static inline int fx_round(int32_t v) { return (int)(int16_t)(((uint32_t)v + 0x8000u) >> 16); }

/* word<<16 (the original builds this with "push w; pop dx; sub ax,ax") */
static inline int32_t fx_from_word(int16_t w) { return (int32_t)((uint32_t)(uint16_t)w << 16); }

/* Turbo Pascal 6-byte Real given as the three registers the compiler loads (AX,BX,DX) -> double */
static inline double real48(uint16_t ax, uint16_t bx, uint16_t dx)
{
    uint8_t b[6] = { ax & 255, ax >> 8, bx & 255, bx >> 8, dx & 255, dx >> 8 };
    if (b[0] == 0) return 0.0;
    uint64_t m = 0;
    for (int i = 5; i >= 1; i--) m = m << 8 | b[i];
    int neg = (int)(m >> 39) & 1;
    m &= (1ULL << 39) - 1;
    double v = ldexp(1.0 + (double)m / 549755813888.0, b[0] - 0x81);
    return neg ? -v : v;
}

/* 1008:27bd - Trunc(r*65536 + 0.5) */
static inline int32_t real_to_fx(double r) { return (int32_t)floor(r * 65536.0 + 0.5); }
#endif
