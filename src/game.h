/* Shared game state: the records and globals of the original Pascal program. Field comments give the
 * original record offsets / DS addresses so the decompilation can be followed side by side. */
#ifndef GAME_H
#define GAME_H
#include <stdint.h>
#include "fixed.h"

/* ---------------------------------------------------------------- ball (TBall at DS:9800) */
typedef struct {
    int16_t  x0, y0, x1, y1;       /* +0 +2 +4 +6  start / target on the ground */
    int16_t  inc_ax, inc_ay;       /* +8 +a   Bresenham step when error <  0 */
    int16_t  inc_bx, inc_by;       /* +c +e   Bresenham step when error >= 0 */
    int16_t  err, d_a, d_b;        /* +10 +12 +14 */
    int16_t  x, y;                 /* +16 +18 current ground position */
    int16_t  dx, dy;               /* +1a +1c target - start */
    uint16_t steps_done;           /* +1e */
    uint16_t nsteps;               /* +20 ground steps in the arc */
    uint16_t t;                    /* +22 phase of the arc, degrees 0..180 */
    int16_t  h;                    /* +24 height above ground */
    uint16_t amp;                  /* +26 arc amplitude */
    uint32_t acc;                  /* +28 16.16 accumulator */
    uint32_t inc;                  /* +2c 16.16 increment */
    int32_t  slope;                /* +30 dx/dy as 16.16 */
    uint8_t  speed;                /* +34 sub-steps per frame */
    uint8_t  bounces;              /* +35 */
    uint8_t  spr_ball, spr_shadow; /* +36 +37 sprite handles */
    uint8_t  past_apex;            /* +38 t > 90 */
    uint8_t  dir;                  /* +39 (DS:9839) ball travelling towards the upper side */
    uint8_t  mode_b;               /* +3a */
    uint8_t  alive;                /* +3b (DS:983b) */
    uint16_t spr_id;               /* +3c current rotation sprite 0x316..0x31c */
    uint16_t spr_tick;             /* +3e */
    int16_t *rec_ball;             /* +40 replay buffer: {x, y, sprite} per frame */
    int16_t *rec_shadow;           /* +44 */
} TBall;

extern TBall ball;                 /* DS:9800 */

/* values copied from the ball at the end of every step (DS:9b38..9b4e) */
extern int16_t g_h, g_amp, g_x, g_y;          /* 9b38 9b3a 9b3c 9b3e */
extern int16_t g_bounce_x, g_bounce_y;        /* 9b40 9b42 */
extern int16_t g_first_x1;                    /* 9b44 target x of the first arc after a hit */
extern int16_t g_x1, g_y1;                    /* 9b48 9b4a */
extern int16_t g_y_pred;                      /* 9b4e */
extern uint8_t g_court_type;                  /* 9518 */
extern uint8_t g_rally;                       /* 9b2d */
extern uint8_t g_doubles;                     /* 9b2c */
extern uint8_t g_out_flag;                    /* 9b66 */
extern uint8_t g_first_serve;                 /* 9b2e */
extern int32_t g_line_k;                      /* 9b5e side line slope (16.16) */
extern uint16_t g_rec_idx;                    /* 9b62 replay frame being recorded */

void  ball_tables_init(void);                 /* 1000:19d8 */
uint32_t ball_tables_checksum(void);          /* network play: both machines must agree */
void  ball_init(TBall *b, uint8_t shadow, uint8_t ball_spr);   /* 1000:1a76 */
void  ball_throw(TBall *b, uint8_t kind, uint16_t h0, int16_t amp, int16_t y1, int16_t x1, int16_t y0, int16_t x0); /* 1afc */
void  ball_hit(TBall *b, uint8_t kind, int16_t amp, int16_t y1, int16_t x1);    /* 2202 */
int   ball_step(TBall *b, int live);          /* 1da9: returns nonzero while the ball is alive */
int   ball_draw(TBall *b);                    /* 2233 */
int   ball_x_at_y(const TBall *b, int y);     /* 33e3 */
void  ball_net(TBall *b);                     /* 31df */
int   ball_out(TBall *b);                     /* 326d */

/* ---------------------------------------------------------------- sound */
void  snd_play(int id);                       /* 1000:011b */
int   snd_busy(void);                         /* 1000:0140 */
#endif
