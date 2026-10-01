/* Player controllers. Each one decides, from the current animation, the control bits and the ball state,
 * which animation the player goes to next. Result = (animation << 8) | flag (flag 1 = start new animation).
 * Transcribed from 1000:408b/486a (human, near/far side) and 1000:5049/5a42 (CPU, near/far side). */
#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include "game.h"
#include "sprites.h"
#include "platform.h"

/* input bits (TENNIS.OPT key order: up, down, left, right, button) */
enum { KUP = 1, KDOWN = 2, KLEFT = 4, KRIGHT = 8, BTN1 = 0x10 };

/* which of up/down wins when both are held (the two human routines differ only here) */
static int lr_sel(unsigned in, int up)
{
    if (!up) return (in & KUP) ? 1 : (in & KDOWN) ? 2 : 0;
    return (in & KDOWN) ? 2 : (in & KUP) ? 1 : 0;
}

static uint16_t pick3(unsigned in, int up, uint16_t none, uint16_t right, uint16_t left)
{
    int s = lr_sel(in, up);
    return s == 0 ? none : s == 2 ? right : left;
}

uint16_t ctrl_human(TPlayer *p, int up)
{
    unsigned in = p->input;
    Sprite *s = spr_get(p->spr);
    uint16_t ret;
    uint8_t anim = p->anim;

    if (!s) return 0x1801;

    if (!(in & BTN1)) {
        switch (anim) {
        case 0x15: return 0x1500;
        case 0x11: case 0x13: {
            int a = anim == 0x13;                   /* 0x13 = same family shifted by 2 */
            int late = timer_elapsed(p->timer_b) >= 0xbfff4;
            /* ids: [none-case][R][L] for the three "ball position" groups */
            static const uint16_t t11[3][3] = { {0x1201, 0x2801, 0x2301}, {0x1a01, 0x2a01, 0x2401}, {0x1901, 0x2901, 0x2201} };
            static const uint16_t t13[3][3] = { {0x1401, 0x2b01, 0x2601}, {0x1c01, 0x2d01, 0x2701}, {0x1b01, 0x2c01, 0x2501} };
            static const uint16_t l11[3] = { 0x3a01, 0x3c01, 0x3b01 };
            static const uint16_t l13[3] = { 0x3d01, 0x3f01, 0x3e01 };
            int g;                                  /* 0: neither up nor down, 1: group for ball-left, 2: for ball-right */
            if (!(in & KRIGHT)) {
                if (!(in & KLEFT)) g = 0;
                else g = g_x < 0xd1 ? 1 : 2;
            } else g = g_x < 0xd2 ? 2 : 1;
            const uint16_t (*t)[3] = a ? t13 : t11;
            uint16_t none = late ? (a ? l13[g] : l11[g]) : t[g][0];
            return pick3(in, up, none, t[g][1], t[g][2]);
        }
        case 0x16: {
            int sel = lr_sel(in, up);
            if (sel == 0) return 0x1600;
            if (sel == 2) return (in & KLEFT) ? 0x1d01 : (in & KRIGHT) ? 0x1e01 : 0x1701;
            return (in & KLEFT) ? 0x1f01 : (in & KRIGHT) ? 0x2101 : 0x2001;
        }
        case 0x2e: case 0x32: case 0x36: {
            static const uint16_t t[3][3] = { {0x2f01, 0x3101, 0x3001}, {0x3301, 0x3501, 0x3401}, {0x3701, 0x3901, 0x3801} };
            const uint16_t *r = t[anim == 0x2e ? 0 : anim == 0x32 ? 1 : 2];
            if (!(in & KRIGHT)) {
                if (!(in & KLEFT)) return r[0];
                return g_x < 0xd1 ? r[1] : r[2];
            }
            return g_x < 0xd2 ? r[2] : r[1];
        }
        default:
            if (!(in & KRIGHT)) {
                if (!(in & KLEFT)) return pick3(in, up, 0x1800, 0x401, 0x301);
                int sel = lr_sel(in, up);
                if (sel == 0) {
                    if (anim == 2) return timer_elapsed(p->timer_a) < 0x5fffb ? 0x201 : 0xa01;
                    if (anim == 10) return 0xa01;
                    timer_start(p->timer_a);
                    return 0x201;
                }
                return sel == 2 ? 0x701 : 0x801;
            } else {
                int sel = lr_sel(in, up);
                if (sel == 0) {
                    if (anim == 1) return timer_elapsed(p->timer_a) < 0x5fffb ? 0x101 : 0x901;
                    if (anim == 9) return 0x901;
                    timer_start(p->timer_a);
                    return 0x101;
                }
                return sel == 2 ? 0x601 : 0x501;
            }
        }
    }

    /* swing button held */
    if (anim == 0x15) return 0x1601;
    if (anim == 0x11 || anim == 0x13 || anim == 0x2e || anim == 0x32 || anim == 0x36 || anim == 0x16)
        return (uint16_t)anim << 8;
    if (!up) {
        if (g_h < 0x3d || g_y < 0x6a) {
            int bx = ball_x_at_y(&ball, s->y), c = (s->w >> 1) + s->x;
            if (s->y < 0xb4) return c < bx ? 0x3201 : 0x3601;
            ret = c < bx ? 0x1101 : 0x1301;
            timer_start(p->timer_b);
            return ret;
        }
        return 0x2e01;
    }
    if (g_h < 0x3d || g_y > 0xd1) {
        int bx = ball_x_at_y(&ball, s->y), c = (s->w >> 1) + s->x;
        if (s->y < 0x79) {
            ret = bx < c ? 0x1101 : 0x1301;
            timer_start(p->timer_b);
            return ret;
        }
        return bx < c ? 0x3201 : 0x3601;
    }
    return 0x2e01;
}

/* ---------------------------------------------------------------- CPU */

static uint16_t rnd15(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f)
{
    int r = rnd(0xf);
    if (r == 0xe) return a;
    if (r == 0xd) return b;
    if (r == 0xc) return c;
    if (r >= 8 && r <= 0xb) return d;
    if (r >= 4 && r <= 7) return e;
    return f;
}

static uint16_t rnd3(uint16_t a, uint16_t b, uint16_t c)     /* 2 -> a, 1 -> b, else c */
{
    int r = rnd(3);
    return r == 2 ? a : r == 1 ? b : c;
}

/* anim 0x16 (waiting for the ball to come down after a toss) - same on both sides */
static uint16_t cpu_anim16(TPlayer *p)
{
    if (!ball.past_apex || (uint16_t)(g_amp - 3) <= (uint16_t)g_h) return 0x1600;
    if (!(p->cpu & 8) || !g_first_serve) {
        if (!(p->cpu & 4) && !(p->cpu & 8)) return 0x1701;
        return rnd3(0x1d01, 0x1e01, 0x1701);
    }
    int r = rnd(10);
    if (r == 6) return 0x1f01;
    if (r == 5) return 0x2101;
    if (r == 4) return 0x2001;
    return 0x1600;
}

/* shared part of both CPU routines for the "running" animations */
static uint16_t cpu_run(TPlayer *p, const Sprite *s, int up)
{
    uint8_t anim = p->anim;
    int cond;
    int inx = up ? (s->x < 0x119 && 0x81 < s->x) : (s->x < 0x137 && 99 < s->x);
    switch (anim) {
    case 0x11: case 0x13: {
        int is13 = anim == 0x13;
        if (g_out_flag) return is13 ? 0x1401 : 0x1201;
        cond = up ? g_y < s->y + 0xb : s->y - 0xf < g_y;
        if (!cond) return is13 ? 0x1300 : 0x1100;
        if (!(p->cpu & 2) || timer_elapsed(p->timer_b) < 0xbfff5) {
            if (!(p->cpu & 1) && !(p->cpu & 2)) {
                if (is13) return inx ? 0x1401 : 0x1c01;
                return inx ? 0x1201 : 0x1a01;
            }
            return is13 ? rnd15(0x2b01, 0x2c01, 0x2d01, 0x1b01, 0x1c01, 0x1401)
                        : rnd15(0x2801, 0x2901, 0x2a01, 0x1901, 0x1a01, 0x1201);
        }
        return is13 ? rnd15(0x2b01, 0x2c01, 0x2d01, 0x3e01, 0x3f01, 0x3d01)
                    : rnd15(0x2801, 0x2901, 0x2a01, 0x3b01, 0x3c01, 0x3a01);
    }
    case 0x32: case 0x36: case 0x2e: {
        int fam = anim == 0x32 ? 0 : anim == 0x36 ? 1 : 2;
        uint16_t a2, a1, a0;
        if (fam == 0) { a2 = 0x3401; a1 = 0x3501; a0 = 0x3301; }
        else if (fam == 1) { a2 = 0x3801; a1 = 0x3901; a0 = 0x3701; }
        else { a2 = 0x3001; a1 = 0x3101; a0 = 0x2f01; }
        if (g_out_flag) return a0;
        cond = fam == 2 ? (up ? g_y < s->y + 0xf : s->y - 0xf < g_y)
                        : (up ? g_y < s->y + 0xc : s->y - 0xc < g_y);
        if (!cond) return fam == 0 ? 0x3200 : fam == 1 ? 0x3600 : 0x2e00;
        if (!(p->cpu & 1) && !(p->cpu & 2)) return inx ? a0 : a1;
        return rnd3(a2, a1, a0);
    }
    }
    return 0x1800;
}

uint16_t ctrl_cpu_down(TPlayer *p)
{
    uint16_t ret = 0x1800;
    Sprite *s = spr_get(p->spr);
    if (!s) return ret;
    uint8_t anim = p->anim;

    if (anim == 0x15) return ball.bounces < 2 ? 0x1500 : 0x1601;
    if (anim == 0x16) return cpu_anim16(p);
    if (anim == 0x11 || anim == 0x13 || anim == 0x32 || anim == 0x36 || anim == 0x2e) return cpu_run(p, s, 0);

    if (!g_out_flag && !ball.dir && g_y > 0x73) {
        int bx = ball_x_at_y(&ball, s->y);
        int serve_special = !g_doubles && !g_rally && g_y_pred < s->y && g_y1 < 0xbe && 0x96 < g_y1;
        if (s->x + 5 < bx || s->x < 0x15) {
            bx = ball_x_at_y(&ball, s->y);
            if (bx < s->x + 0x33 || 0x150 < s->x) {
                if (s->y - 0x28 < g_y || (s->y < 0xb4 && g_y > 0x73)) {
                    if (g_y < s->y + 1 && ball.bounces < 2) {
                        if (g_h < 0x3d || g_y < 0x6a) {
                            if (s->y < 0xb4) {
                                bx = ball_x_at_y(&ball, s->y);
                                ret = s->x + 0x1c < bx ? 0x3201 : 0x3601;
                            } else {
                                bx = ball_x_at_y(&ball, s->y);
                                ret = s->x + 0x1c < bx ? 0x1101 : 0x1301;
                                timer_start(p->timer_b);
                            }
                        } else ret = 0x2e01;
                    }
                } else if (serve_special) {
                    ret = 0x301;
                } else if ((!g_doubles && g_y > 0x96) || (g_doubles && g_y > 0xaf)) {
                    if (s->y < 0xb4) {
                        bx = ball_x_at_y(&ball, s->y);
                        ret = s->x + 0x1c < bx ? 0x3201 : 0x3601;
                    } else if (ball.bounces == 0 && s->y - 10 < g_y1) {
                        ret = 0x401;
                    } else {
                        bx = ball_x_at_y(&ball, s->y);
                        ret = s->x + 0x1c < bx ? 0x1101 : 0x1301;
                        timer_start(p->timer_b);
                    }
                }
            } else if (serve_special) {
                ret = 0x501;
            } else if (p->anim == 1) {
                ret = timer_elapsed(p->timer_a) < 0x5fffb ? 0x101 : 0x901;
            } else if (p->anim == 9) {
                ret = 0x901;
            } else if (!g_doubles || !g_rally || s->y > 0xb3) {
                ret = 0x101;
                timer_start(p->timer_a);
            }
        } else if (serve_special) {
            ret = 0x801;
        } else if (p->anim == 2) {
            ret = timer_elapsed(p->timer_a) < 0x5fffb ? 0x201 : 0xa01;
        } else if (p->anim == 10) {
            ret = 0xa01;
        } else if (!g_doubles || !g_rally || s->y > 0xb3) {
            ret = 0x201;
            timer_start(p->timer_a);
        }
    } else if (!g_out_flag && !g_rally && ball.dir) {
        int e;
        if (s->y < 0xbe) e = g_first_x1 < 0xa0 ? 0x96 : g_first_x1 < 0xfa ? 0xbe : 0xdc;
        else e = 0xbe;
        if (s->y < 0xdc && 0xa2 < s->y) {
            if (s->x < e - 10) ret = 0x501;
            else if (e + 10 < s->x) ret = 0x801;
            else ret = 0x301;
        } else if (s->x < e - 10) ret = 0x101;
        else if (e + 10 < s->x) ret = 0x201;
        else if (0xf0 < s->y) ret = 0x301;
    }
    return ret;
}

uint16_t ctrl_cpu_up(TPlayer *p)
{
    uint16_t ret = 0x1800;
    Sprite *s = spr_get(p->spr);
    if (!s) return ret;
    uint8_t anim = p->anim;

    if (anim == 0x15) return ball.bounces < 2 ? 0x1500 : 0x1601;
    if (anim == 0x16) return cpu_anim16(p);
    if (anim == 0x11 || anim == 0x13 || anim == 0x32 || anim == 0x36 || anim == 0x2e) return cpu_run(p, s, 1);

    if (!g_out_flag && ball.dir && g_y < 200) {
        int serve_special = !g_doubles && !g_rally && g_y1 < 0x96 && 0x78 < g_y1 && s->y < 0x6e;
        int bx = ball_x_at_y(&ball, s->y);
        if (s->x + 5 < bx || s->x < 0x15) {
            bx = ball_x_at_y(&ball, s->y);
            if (bx < s->x + 0x33 || 0x150 < s->x) {
                if (g_y < s->y + 0x28 || (0x78 < s->y && g_y < 0xbe)) {
                    if (s->y - 1 < g_y && ball.bounces < 2) {
                        if (g_h < 0x3d || 0xd1 < g_y) {
                            bx = ball_x_at_y(&ball, s->y);
                            if (s->y < 0x79) {
                                ret = bx < s->x + 0x1c ? 0x1101 : 0x1301;
                                timer_start(p->timer_b);
                            } else {
                                ret = bx < s->x + 0x1c ? 0x3201 : 0x3601;
                            }
                        } else ret = 0x2e01;
                    }
                } else if (serve_special) {
                    ret = 0x401;
                } else if ((!g_doubles && g_y < 0xbe) || (g_doubles && g_y < 0x7b)) {
                    if (s->y < 0x79) {
                        if (ball.bounces == 0 && g_y1 < s->y + 10) {
                            ret = 0x301;
                        } else {
                            bx = ball_x_at_y(&ball, s->y);
                            ret = bx < s->x + 0x1c ? 0x1101 : 0x1301;
                        }
                    } else {
                        bx = ball_x_at_y(&ball, s->y);
                        ret = bx < s->x + 0x1c ? 0x3201 : 0x3601;
                    }
                    timer_start(p->timer_b);
                }
            } else if (serve_special) {
                ret = 0x601;
            } else if (p->anim == 1) {
                ret = timer_elapsed(p->timer_a) < 0x5fffb ? 0x101 : 0x901;
            } else if (p->anim == 9) {
                ret = 0x901;
            } else if (!g_doubles || !g_rally || s->y < 0x79) {
                ret = 0x101;
                timer_start(p->timer_a);
            }
        } else if (serve_special) {
            ret = 0x701;
        } else if (p->anim == 2) {
            ret = timer_elapsed(p->timer_a) < 0x5fffb ? 0x201 : 0xa01;
        } else if (p->anim == 10) {
            ret = 0xa01;
        } else if (!g_doubles || !g_rally || s->y < 0x79) {
            ret = 0x201;
            timer_start(p->timer_a);
        }
    } else if (!g_out_flag && !g_rally && !ball.dir) {
        int e;
        if (s->y < 0x65) e = 0xbe;
        else if (g_first_x1 < 0x8c) e = 0x96;
        else if (g_first_x1 < 0x118) e = 0xbe;
        else e = 0xdc;
        if (s->y < 0x65 || 0x90 < s->y) {
            if (s->x < e - 10) ret = 0x101;
            else if (e + 10 < s->x) ret = 0x201;
            else if (s->y < 0x5a) ret = 0x401;
        } else if (s->x < e - 10) ret = 0x601;
        else if (e + 10 < s->x) ret = 0x701;
        else ret = 0x401;
    }
    return ret;
}
