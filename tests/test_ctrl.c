/* Synthetic states exercise the replacement controller and shot policies.
 * No original game files or extracted tables are required. */
#include "../src/player.h"
#include "../src/sprites.h"
#include "../src/dsimg.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t data[65536];
uint8_t *ds_data = data;
TBall ball;
int16_t g_h, g_amp, g_x, g_y, g_y_pred, g_y1, g_first_x1;
uint8_t g_rally, g_first_serve;
uint8_t g_out_flag;
uint8_t g_doubles;
TPlayer *pl_near[2], *pl_far[2];
static Sprite sprite;
static unsigned elapsed;
static int hit, hit_y, hit_x, sound;

Sprite *spr_get(int handle) { return handle == 1 ? &sprite : NULL; }
uint32_t timer_elapsed(int id) { (void)id; return elapsed; }
void timer_start(int id) { (void)id; elapsed = 0; }
uint16_t rnd(uint16_t limit) { return limit ? limit - 1 : 0; }
int ball_x_at_y(const TBall *b, int y) { (void)y; return b->x1; }
void ball_hit(TBall *b, uint8_t speed, int16_t amplitude, int16_t y, int16_t x)
{
    (void)b; (void)amplitude;
    assert(speed >= 2 && speed <= 4);
    hit++; hit_y = y; hit_x = x;
}
void snd_play(int id) { sound = id; }

int main(void)
{
    TPlayer p;
    memset(&p, 0, sizeof p);
    p.spr = 1; p.anim = 24;
    sprite.x = 180; sprite.y = 215; sprite.w = 56;
    data[0x333 + 1] = 1; data[0x333 + 2] = 255;
    p.input = 8;
    assert(ctrl_human(&p, 0) == 0x0101);
    p.input = 4;
    assert(ctrl_human(&p, 0) == 0x0201);
    p.input = 12;
    assert(ctrl_human(&p, 0) == 0x0101); /* Right has priority. */
    p.anim = 21; p.input = 16;
    assert(ctrl_human(&p, 0) == 0x1601);
    p.anim = 22;
    assert(ctrl_human(&p, 0) == 0x1600);
    p.input = 0;
    assert(ctrl_human(&p, 0) == 0x1600); /* Toss waits for a direction. */
    p.anim = 21; ball.bounces = 1;
    assert(ctrl_cpu_down(&p) == 0x1500);
    ball.bounces = 2;
    assert(ctrl_cpu_down(&p) == 0x1601);
    p.anim = 22; ball.past_apex = 1; g_h = 30;
    assert(ctrl_cpu_up(&p) == 0x1701);
    p.anim = 17; p.input = 0; elapsed = 0;
    assert(ctrl_human(&p, 0) == 0x1201);
    p.input = 1;
    assert(ctrl_human(&p, 0) == 0x2301);
    p.input = 2;
    assert(ctrl_human(&p, 0) == 0x2801);
    p.anim = 46;
    assert(ctrl_human(&p, 0) == 0x2f01);
    p.anim = 1; p.input = 8; elapsed = 393210;
    assert(ctrl_human(&p, 0) == 0x0101);
    elapsed = 393211;
    assert(ctrl_human(&p, 0) == 0x0901);
    p.anim = 17; p.input = 0; elapsed = 786419;
    assert(ctrl_human(&p, 0) == 0x1201);
    elapsed = 786420;
    assert(ctrl_human(&p, 0) == 0x3a01);
    ball.dir = 0; ball.x = 140; ball.h = 12; g_y = 105;
    ball_shot(&ball, 18);
    assert(hit == 1 && hit_x == 100 && hit_y == 75 && ball.dir == 1 && sound == 3);
    int before = hit;
    ball_shot(&ball, 24);
    assert(hit == before && ball.dir == 1);
    puts("compatibility: key priority, release timing and stroke targets: OK");
    return 0;
}
