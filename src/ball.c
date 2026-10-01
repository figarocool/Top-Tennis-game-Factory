/* Ball physics. The ball flies along a Bresenham line on the ground while its height follows
 * h = amp * sin(t) with t running 0..180 degrees; every bounce restarts the arc with scaled distance
 * and amplitude (coefficients depend on the court). Original: segment 1000, 19d8..2233, 31df, 326d. */
#include "game.h"
#include "sprites.h"
#include "platform.h"
#include <stdlib.h>
#include <string.h>

TBall ball;
int16_t g_h, g_amp, g_x, g_y, g_bounce_x, g_bounce_y, g_first_x1, g_x1, g_y1, g_y_pred;
uint8_t g_court_type, g_rally, g_doubles, g_out_flag, g_first_serve;
int32_t g_line_k;
uint16_t g_rec_idx;

static int32_t sin_tab[181];                   /* DS:9848 */

void ball_tables_init(void)
{
    for (int t = 0; t <= 180; t++)
        sin_tab[t] = real_to_fx(sin(t * 0.017453292519943295) );
    /* 9b5e: Real constant loaded in the program body (1000:ed2b): AX=f17f BX=4a33 DX=0cfc */
    g_line_k = real_to_fx(real48(0xf17f, 0x4a33, 0x0cfc));
}

/* 1000:1a2e - first t with sin_tab[t] >= v */
static int sin_inverse(int32_t v)
{
    int t = 0;
    for (;;) {
        if (v <= sin_tab[t] || t == 180) break;
        t++;
    }
    return t;
}

void ball_init(TBall *b, uint8_t shadow, uint8_t ball_spr)
{
    b->spr_ball   = ball_spr;
    b->spr_shadow = shadow;
    b->spr_id     = 0x316;
    b->spr_tick   = 0;
    b->alive      = 0;
    b->rec_ball   = malloc(0xffd8);
    b->rec_shadow = malloc(0xffd8);
}

void ball_throw(TBall *b, uint8_t kind, uint16_t h0, int16_t amp, int16_t y1, int16_t x1, int16_t y0, int16_t x0)
{
    b->x0 = x0; b->y0 = y0; b->x1 = x1; b->y1 = y1;

    int diag_x = b->x1 != b->x0;                  /* uVar3  */
    int diag_y = b->y1 != b->y0;                  /* local_6 */
    int ady = b->y1 - b->y0;
    b->dy = ady;
    if (ady < 0) { diag_y = -diag_y; ady = -ady; }
    int adx = b->x1 - b->x0;
    b->dx = adx;
    if (adx < 0) { diag_x = -diag_x; adx = -adx; }

    int major, minor, str_x, str_y;               /* Bresenham along the longer axis */
    if (adx < ady) { str_x = 0;      str_y = diag_y; major = ady; minor = adx; }
    else           { str_x = diag_x; str_y = 0;      major = adx; minor = ady; }
    b->inc_ax = str_x;  b->inc_ay = str_y;        /* +8 +a  */
    b->inc_bx = diag_x; b->inc_by = diag_y;       /* +c +e  */
    b->x = b->x0; b->y = b->y0;
    b->d_a = minor << 1;
    b->d_b = minor * 2 - major * 2;
    b->err = minor * 2 - major;
    b->nsteps = major + 1;
    b->speed = kind;
    b->bounces = 0;
    b->h = 0;
    b->amp = amp;

    int t0;
    if ((int16_t)h0 < (int32_t)b->amp)
        t0 = sin_inverse(fx_div(fx_from_word(h0), fx_from_word(b->amp)));
    else
        t0 = 0x5a;
    int32_t remaining = 0xb4 - t0;
    b->mode_b = (int32_t)b->nsteps < remaining;
    if (b->mode_b) {
        b->acc = 0;
        b->inc = fx_div(fx_from_word(b->nsteps), fx_from_word(remaining));
    } else {
        b->acc = (uint32_t)t0 << 16;
        b->inc = fx_div(fx_from_word(remaining), fx_from_word(b->nsteps));
    }
    b->steps_done = 0;
    b->t = t0;
    b->past_apex = b->t > 0x5a;
    if (b->y1 == b->y0)
        b->slope = fx_from_word(b->x1 - b->x0);
    else
        b->slope = (int32_t)((((int64_t)(int16_t)(b->x1 - b->x0)) << 32) / ((int64_t)(int16_t)(b->y1 - b->y0) << 16));
}

void ball_hit(TBall *b, uint8_t kind, int16_t amp, int16_t y1, int16_t x1)
{
    ball_throw(b, kind, (uint16_t)b->h, amp, y1, x1, b->y, b->x);
}

/* 1000:2233 - show the ball and its shadow; fails when the ball left the playing area */
int ball_draw(TBall *b)
{
    int sy = b->y - b->h;
    if (b->x < 10 || b->x > 0x195 || sy < 10 || sy > 0x120 || b->y < 10 || b->y > 0x120 || b->bounces > 2) {
        if (b->bounces < 2) b->bounces = 2;
        return 0;
    }
    spr_set_def(b->spr_ball, b->spr_id);
    spr_pos(b->spr_ball, sy, b->x);
    spr_pos(b->spr_shadow, b->y, b->x);
    if (++b->spr_tick > 10) {
        b->spr_tick = 0;
        if (++b->spr_id > 0x31c) b->spr_id = 0x316;
    }
    return 1;
}

static void bounce_coeffs(const TBall *b, int live, int32_t *kx, int32_t *ky)
{
    if (g_rally && !b->dir) *kx = real_to_fx(real48(0xcd81, 0xcccc, 0x0ccc));          /* 1.1 */
    else if (g_court_type == 2 || g_court_type == 3 || g_court_type == 4)
        *kx = real_to_fx(real48(0x3381, 0x3333, 0x3333));                                  /* 1.4 */
    else
        *kx = real_to_fx(real48(0x0081, 0x0000, 0x4000));                                  /* 1.5 */
    if (!live) { *ky = 1 << 16; return; }
    if (g_rally && b->dir) *ky = real_to_fx(real48(0xcd82, 0xcccc, 0x0ccc));            /* 2.2 */
    else if (g_court_type == 2 || g_court_type == 4) *ky = real_to_fx(real48(0x6681, 0x6666, 0x6666));  /* 1.8 */
    else *ky = real_to_fx(real48(0x0082, 0x0000, 0x0000));                                 /* 2.0 (court 3 and default) */
}

int ball_step(TBall *b, int live)
{
    int result = 0;
    if (b->t <= 180) {
        uint8_t n = b->speed;
        if (n != 0) {
            for (uint8_t i = 1; ; i++) {
                b->h = (int16_t)fx_round(fx_mul(fx_from_word(b->amp), sin_tab[b->t]));
                if (i == 1) {
                    if (ball_draw(b)) result = 1;
                    else { b->alive = 0; result = 0; goto done; }
                } else result = 1;
                int ground = 1;
                if (!b->mode_b) {
                    b->acc += b->inc;
                    b->t = (uint16_t)fx_round((int32_t)b->acc);
                    b->steps_done++;
                } else {
                    b->t++;
                    b->acc += b->inc;
                    int r = fx_round((int32_t)b->acc);
                    if (r > (int)b->steps_done) b->steps_done++;
                    else ground = 0;
                }
                b->past_apex = b->t > 0x5a;
                if (b->t > 180) break;
                if (ground) {
                    if (b->err < 0) { b->x += b->inc_ax; b->y += b->inc_ay; b->err += b->d_a; }
                    else            { b->x += b->inc_bx; b->y += b->inc_by; b->err += b->d_b; }
                }
                if (i == n) break;
            }
        }
    } else if (b->amp < 2) {
        b->alive = 0;
        result = 0;
    } else {
        /* the arc is over: bounce */
        int bnc = ++b->bounces;
        g_bounce_x = b->x + 3;
        g_bounce_y = b->y - 1;
        int32_t kx, ky;
        bounce_coeffs(b, live, &kx, &ky);
        /* 1008:2845 divides: the ball keeps dx/kx, dy/kx of its run and amp/ky of its height (it loses energy) */
        int16_t nx = (int16_t)(fx_round(fx_div(fx_from_word(b->dx), kx)) + b->x);
        int16_t ny = (int16_t)(fx_round(fx_div(fx_from_word(b->dy), kx)) + b->y);
        int16_t namp = (int16_t)fx_round(fx_div(fx_from_word(b->amp), ky));
        ball_hit(b, b->speed + 1, namp, ny, nx);
        b->bounces = bnc;
        snd_play(2);
        result = 1;
    }
done:
    g_h = b->h; g_amp = b->amp; g_x = b->x; g_y = b->y;
    g_x1 = b->x1; g_y1 = b->y1;
    if (b->bounces == 0) g_first_x1 = b->x1;
    g_y_pred = (int16_t)(fx_round(fx_div(fx_from_word(b->dy), 2 << 16)) + b->y1);
    return result;
}

/* 1000:33e3 - x of the ball path at ground row y */
int ball_x_at_y(const TBall *b, int y)
{
    return fx_round(fx_mul(fx_from_word((int16_t)(y - b->y0)), b->slope)) + b->x0;
}

/* 1000:31df - ball hits the net band while low */
void ball_net(TBall *b)
{
    if (b->alive) return;
    if (!b->dir) {
        if (g_y < 0x99 && g_y > 0x94 && g_h < 0x14) {
            b->alive = 1;
            ball_hit(b, 5, g_h, 0x95, g_x);
            snd_play(1);
        }
    } else if (g_y > 0x97 && g_y < 0x9c && g_h < 0x14) {
        b->alive = 1;
        ball_hit(b, 5, g_h, 0x9b, g_x);
        snd_play(1);
    }
}

/* 1000:326d - did the ball land outside the lines? (plays the OUT sound) */
int ball_out(TBall *b)
{
    int out;
    if (!b->dir) out = !((!g_rally || g_bounce_y < 0xbf) && g_bounce_y < 0xe8);
    else         out = !((!g_rally || g_bounce_y > 0x77) && g_bounce_y > 0x5c);
    if (!out) {
        int d = fx_round(fx_mul(g_line_k, fx_from_word((int16_t)(g_bounce_y - 0x5d))));
        if (!g_doubles) {
            if (g_bounce_x < 0xd1) out = g_bounce_x < 0x81 - d;
            else                   out = d + 0x123 < g_bounce_x;
        } else {
            if (g_bounce_x < 0xd1) out = g_bounce_x < 0x66 - d;
            else                   out = d + 0x13f < g_bounce_x;
        }
    }
    if (out) snd_play(5);
    return out;
}
