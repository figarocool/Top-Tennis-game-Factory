/* A player's racket hits the ball: the animation that is playing (the stroke type) chooses the target
 * point on the opponent's side, the ball speed and the arc amplitude. Original: 1000:259c and 3126. */
#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include "game.h"

static int clamp20(int v) { return v < -0x14 ? -0x14 : v > 0x14 ? 0x14 : v; }

/* strokes 0x12..0x3f */
void ball_shot(TBall *b, uint8_t a)
{
    int bx = b->x, bh = b->h;
    int near = bx < 0xd1;                  /* ball on the left half */
    int far_ = bx > 0xd1;                  /* ball on the right half */
    int d = (g_amp - 5) - bh;
    int k_slow = clamp20(d / 2);           /* local_6 */
    int k_fast = clamp20(d * 2);           /* local_8 */

    if (!b->dir) {                         /* ball goes to the far side */
        b->dir = 1;
        int m = (g_y - 0xe2) / 3;          /* iVar1 */
        int cross = 0xd1 - (bx - 0xd1) / 2;
        int side = far_ * 0x8c + 0x8c;
        switch (a) {
        case 0x17: ball_hit(b, 2, bh, k_slow + 0x7a, near * 0x50 + 0xa5); break;
        case 0x20: ball_hit(b, 3, bh, k_fast + 0x7a, near * 0x50 + 0xa5); break;
        case 0x1f: ball_hit(b, 3, bh, k_fast + 0x7a, near * 0x50 + 0x8c); break;   /* Byte arithmetic in the original: 0x50*n - 0x74 wraps to + 0x8c */
        case 0x1d: ball_hit(b, 2, bh, k_slow + 0x7a, near * 0x50 + 0x8c); break;
        case 0x21: ball_hit(b, 3, bh, k_fast + 0x7a, near * 0x50 + 200); break;
        case 0x1e: ball_hit(b, 2, bh, k_slow + 0x7a, near * 0x50 + 200); break;
        case 0x12: ball_hit(b, 2, 0x23, m + 0x73, bx - 0x28 + (bx > 0xf0) * -0x14); break;
        case 0x14: ball_hit(b, 2, 0x23, m + 0x73, bx + 0x28 + (bx > 0xdc) * -0x14 + (bx < 0xb4) * 0x14); break;
        case 0x3a: ball_hit(b, 3, 0x23, m + 0x73, bx - 0x28 + (bx > 0xf0) * -0x14); break;
        case 0x3d: ball_hit(b, 3, 0x23, m + 0x73, bx + 0x28 + (bx > 0xdc) * -0x14 + (bx < 0xb4) * 0x14); break;
        case 0x1a: case 0x1c: ball_hit(b, 2, 0x23, m + 0x73, side); break;
        case 0x3c: case 0x3f: ball_hit(b, 3, 0x23, m + 0x73, side); break;
        case 0x19: case 0x1b: ball_hit(b, 2, 0x23, m + 0x73, cross); break;
        case 0x3b: case 0x3e: ball_hit(b, 3, 0x23, m + 0x73, cross); break;
        case 0x23: ball_hit(b, 2, 0x55, m + 0x6e, bx - 0x28); break;
        case 0x26: ball_hit(b, 2, 0x55, m + 0x6e, bx + 0x32); break;
        case 0x24: case 0x27: ball_hit(b, 2, 0x55, m + 0x6e, side); break;
        case 0x22: case 0x25: ball_hit(b, 2, 0x55, m + 0x6e, cross); break;
        case 0x28: ball_hit(b, 2, 0x2b, 0x87, bx - 0x28); break;
        case 0x2b: ball_hit(b, 2, 0x2b, 0x87, bx + 0x32); break;
        case 0x2a: case 0x2d: ball_hit(b, 2, 0x2b, 0x87, side); break;
        case 0x29: case 0x2c: ball_hit(b, 2, 0x2b, 0x87, cross); break;
        case 0x2f: ball_hit(b, 4, bh, 0x6e, bx); break;
        case 0x31: ball_hit(b, 4, bh, 0x6e, side); break;
        case 0x30: ball_hit(b, 4, bh, 0x6e, cross); break;
        case 0x33: ball_hit(b, 4, bh, 0x6e, bx - 0x28); break;
        case 0x37: ball_hit(b, 4, bh, 0x6e, bx + 0x28); break;
        case 0x35: case 0x39: ball_hit(b, 4, bh, 0x6e, side); break;
        case 0x34: case 0x38: ball_hit(b, 4, bh, 0x6e, cross); break;
        default: b->dir = 0; break;
        }
    } else {                               /* ball goes to the near side */
        b->dir = 0;
        int m = (g_y - 0x5d) / 2;
        int cross = 0xd1 - (bx - 0xd1);
        int side = far_ * 200 + 0x6e;
        switch (a) {
        case 0x17: ball_hit(b, 2, bh, 0xb9 - k_slow, near * 0x5a + 0xa0); break;
        case 0x20: ball_hit(b, 3, bh, 0xb9 - k_fast, near * 0x5a + 0xa0); break;
        case 0x1f: ball_hit(b, 3, bh, 0xb9 - k_fast, near * 0x5f + 0x78); break;
        case 0x1d: ball_hit(b, 2, bh, 0xb9 - k_slow, near * 0x5f + 0x78); break;
        case 0x21: ball_hit(b, 3, bh, 0xb9 - k_fast, near * 0x55 + 200); break;
        case 0x1e: ball_hit(b, 2, bh, 0xb9 - k_slow, near * 0x55 + 200); break;
        case 0x12: ball_hit(b, 2, 0x23, m + 200, bx + 0x28); break;
        case 0x14: ball_hit(b, 2, 0x23, m + 200, bx - 0x28); break;
        case 0x3a: ball_hit(b, 3, 0x23, m + 200, bx + 0x28); break;
        case 0x3d: ball_hit(b, 3, 0x23, m + 200, bx - 0x28); break;
        case 0x1a: case 0x1c: ball_hit(b, 2, 0x23, m + 200, side); break;
        case 0x3c: case 0x3f: ball_hit(b, 3, 0x23, m + 200, side); break;
        case 0x19: case 0x1b: ball_hit(b, 2, 0x23, m + 200, cross); break;
        case 0x3b: case 0x3e: ball_hit(b, 3, 0x23, m + 200, cross); break;
        case 0x23: ball_hit(b, 2, 0x55, m + 0xd2, bx + 0x28); break;
        case 0x26: ball_hit(b, 2, 0x55, m + 0xd2, bx - 0x28); break;
        case 0x24: case 0x27: ball_hit(b, 2, 0x55, m + 0xd2, side); break;
        case 0x22: case 0x25: ball_hit(b, 2, 0x55, m + 0xd2, cross); break;
        case 0x28: ball_hit(b, 2, 0x28, 0xaa, bx + 0x28); break;
        case 0x2b: ball_hit(b, 2, 0x28, 0xaa, bx - 0x28); break;
        case 0x2a: case 0x2d: ball_hit(b, 2, 0x28, 0xaa, far_ * 0xbe + 0x73); break;
        case 0x29: case 0x2c: ball_hit(b, 2, 0x28, 0xaa, cross); break;
        case 0x2f: ball_hit(b, 4, bh, 0xd2, bx); break;
        case 0x31: ball_hit(b, 4, bh, 0xd2, side); break;
        case 0x30: ball_hit(b, 4, bh, 0xd2, cross); break;
        case 0x33: ball_hit(b, 4, bh, 0xd2, bx + 0x28); break;
        case 0x37: ball_hit(b, 4, bh, 0xd2, bx - 0x28); break;
        case 0x35: case 0x39: ball_hit(b, 4, bh, 0xd2, side); break;
        case 0x34: case 0x38: ball_hit(b, 4, bh, 0xd2, cross); break;
        default: b->dir = 1; break;
        }
    }
    shot_sound(a);
}

/* 1000:3126 */
void shot_sound(uint8_t a)
{
    switch (a) {
    case 0x17: case 0x1d: case 0x1e: case 0x12: case 0x14: case 0x1a: case 0x1c: case 0x19: case 0x1b:
    case 0x23: case 0x26: case 0x24: case 0x27: case 0x22: case 0x25: case 0x33: case 0x37: case 0x35:
    case 0x39: case 0x34: case 0x38: case 0x28: case 0x2b: case 0x2a: case 0x2d: case 0x29: case 0x2c:
        snd_play(3); break;
    case 0x20: case 0x1f: case 0x21: case 0x3a: case 0x3d: case 0x3c: case 0x3f: case 0x3b: case 0x3e:
    case 0x2f: case 0x31: case 0x30:
        snd_play(4); break;
    }
}
