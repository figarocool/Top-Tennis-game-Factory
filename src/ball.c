/* Compatibility physics expressed as a sampled arc plus a closed-form pixel
 * path. The model preserves reference rounding and frame order. */
#include "game.h"
#include "sprites.h"
#include <stdlib.h>
#include <string.h>

TBall ball;
int16_t g_h, g_amp, g_x, g_y, g_bounce_x, g_bounce_y, g_first_x1, g_x1, g_y1, g_y_pred;
uint8_t g_court_type, g_rally, g_doubles, g_out_flag, g_first_serve;
int32_t g_line_k;
uint16_t g_rec_idx;
static int32_t heights[181];

void ball_tables_init(void)
{
    for (unsigned angle = 0; angle <= 180; angle++)
        heights[angle] = real_to_fx(sin(angle * 0.017453292519943295));
    g_line_k = real_to_fx(19.0 / 69.0);
}
uint32_t ball_tables_checksum(void)
{
    uint32_t h = 2166136261u;
    for (unsigned angle = 0; angle <= 180; angle++) h = (h ^ (uint32_t)heights[angle]) * 16777619u;
    return (h ^ (uint32_t)g_line_k) * 16777619u;
}
static unsigned ascending_phase(int32_t height)
{
    if (height <= 0) return 0;
    if (height > heights[90]) return 180;
    unsigned lo = 0, hi = 90;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (heights[mid] < height) lo = mid + 1; else hi = mid;
    }
    return lo;
}
static void publish(TBall *b)
{
    g_x = b->x; g_y = b->y; g_h = b->h; g_amp = b->amp;
    g_x1 = b->x1; g_y1 = b->y1;
    if (!b->bounces) g_first_x1 = b->x1;
    g_y_pred = fx_round(fx_div(fx_from_word(b->dy), fx_from_word(2))) + b->y1;
}
void ball_init(TBall *b, uint8_t shadow, uint8_t sprite)
{
    b->spr_shadow = shadow; b->spr_ball = sprite; b->spr_id = 790; b->spr_tick = 0; b->alive = 0;
    if (!b->rec_ball) b->rec_ball = calloc(10916u, 6);
    if (!b->rec_shadow) b->rec_shadow = calloc(10916u, 6);
}
void ball_throw(TBall *b, uint8_t kind, uint16_t height, int16_t amplitude,
                int16_t y1, int16_t x1, int16_t y0, int16_t x0)
{
    b->x0 = b->x = x0; b->y0 = b->y = y0; b->x1 = x1; b->y1 = y1;
    b->dx = x1 - x0; b->dy = y1 - y0;
    int ax = abs(b->dx), ay = abs(b->dy);
    b->path_x_major = ax >= ay; b->path_major = ax > ay ? ax : ay;
    b->path_minor = ax > ay ? ay : ax; b->path_step = 0;
    b->speed = kind; b->bounces = 0; b->h = 0; b->amp = amplitude;
    unsigned phase = (int16_t)height < b->amp ? ascending_phase(fx_div(fx_from_word(height), fx_from_word(b->amp))) : 90;
    unsigned duration = 180 - phase, distance = b->path_major + 1;
    b->mode_b = distance < duration;
    b->steps_done = 0; b->nsteps = distance; b->t = phase; b->past_apex = phase > 90;
    b->acc = b->mode_b ? 0 : phase * 65536u;
    b->inc = b->mode_b ? fx_div(fx_from_word(distance), fx_from_word(duration)) :
                        fx_div(fx_from_word(duration), fx_from_word(distance));
    b->slope = b->dy ? (int32_t)((int64_t)b->dx * 65536 / b->dy) : fx_from_word(b->dx);
}
void ball_hit(TBall *b, uint8_t kind, int16_t amplitude, int16_t y1, int16_t x1)
{
    ball_throw(b, kind, (uint16_t)b->h, amplitude, y1, x1, b->y, b->x);
}
int ball_draw(TBall *b)
{
    int screen_y = b->y - b->h;
    int visible = b->x >= 10 && b->x <= 405 && b->y >= 10 && b->y <= 288 &&
                  screen_y >= 10 && screen_y <= 288 && b->bounces <= 2;
    if (!visible) { if (b->bounces < 2) b->bounces = 2; return 0; }
    spr_set_def(b->spr_ball, b->spr_id); spr_pos(b->spr_ball, screen_y, b->x); spr_pos(b->spr_shadow, b->y, b->x);
    if (++b->spr_tick > 10) { b->spr_tick = 0; b->spr_id = 790 + (b->spr_id - 790 + 1) % 7; }
    return 1;
}
static void advance_path(TBall *b)
{
    unsigned step = ++b->path_step;
    unsigned minor = b->path_major ? (step * b->path_minor + b->path_major / 2) / b->path_major : 0;
    int px = b->path_x_major ? step : minor, py = b->path_x_major ? minor : step;
    b->x = b->x0 + (b->dx < 0 ? -px : b->dx ? px : 0);
    b->y = b->y0 + (b->dy < 0 ? -py : b->dy ? py : 0);
}
static int advance_phase(TBall *b)
{
    b->acc += b->inc;
    if (!b->mode_b) { b->t = fx_round((int32_t)b->acc); b->steps_done++; return 1; }
    b->t++;
    int ground = fx_round((int32_t)b->acc) > b->steps_done;
    if (ground) b->steps_done++;
    return ground;
}
static void bounce(TBall *b, int live)
{
    unsigned count = b->bounces + 1;
    g_bounce_x = b->x + 3; g_bounce_y = b->y - 1;
    double distance_loss = g_rally && !b->dir ? 1.1 : g_court_type >= 2 && g_court_type <= 4 ? 1.4 : 1.5;
    double height_loss = !live ? 1.0 : g_rally && b->dir ? 2.2 : g_court_type == 2 || g_court_type == 4 ? 1.8 : 2.0;
    int x = b->x + fx_round(fx_div(fx_from_word(b->dx), real_to_fx(distance_loss)));
    int y = b->y + fx_round(fx_div(fx_from_word(b->dy), real_to_fx(distance_loss)));
    int height = fx_round(fx_div(fx_from_word(b->amp), real_to_fx(height_loss)));
    ball_hit(b, b->speed + 1, height, y, x); b->bounces = count; snd_play(2);
}
int ball_step(TBall *b, int live)
{
    int running = 0;
    if (b->t > 180) {
        if (b->amp < 2) b->alive = 0;
        else { bounce(b, live); running = 1; }
    } else {
        for (unsigned tick = 0; tick < b->speed; tick++) {
            b->h = fx_round(fx_mul(fx_from_word(b->amp), heights[b->t]));
            if (!tick && !ball_draw(b)) { b->alive = 0; break; }
            running = 1;
            int ground = advance_phase(b); b->past_apex = b->t > 90;
            if (b->t > 180) break;
            if (ground) advance_path(b);
        }
    }
    publish(b); return running;
}
int ball_x_at_y(const TBall *b, int y)
{
    return b->x0 + fx_round(fx_mul(fx_from_word(y - b->y0), b->slope));
}
void ball_net(TBall *b)
{
    int lo = b->dir ? 151 : 148, hi = b->dir ? 156 : 153;
    if (b->alive || g_y <= lo || g_y >= hi || g_h >= 20) return;
    b->alive = 1; ball_hit(b, 5, g_h, b->dir ? 155 : 149, g_x); snd_play(1);
}
int ball_out(TBall *b)
{
    int limit = b->dir ? (g_rally ? 119 : 92) : (g_rally ? 191 : 232);
    int inside_depth = b->dir ? g_bounce_y > limit : g_bounce_y < limit;
    int width = fx_round(fx_mul(g_line_k, fx_from_word(g_bounce_y - 93)));
    int left = (g_doubles ? 102 : 129) - width, right = (g_doubles ? 319 : 291) + width;
    int inside_width = g_bounce_x < 209 ? g_bounce_x >= left : g_bounce_x <= right;
    int out = !inside_depth || !inside_width;
    if (out) snd_play(5);
    return out;
}
