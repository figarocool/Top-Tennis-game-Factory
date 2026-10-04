/* Player animation is a frame clock plus asset-defined movement. Recordings use
 * three signed words per frame, independently of the runtime player layout. */
#include "player.h"
#include "joy.h"
#include "net.h"
#include "dsimg.h"
#include "sprites.h"
#include "platform.h"
#include <stdlib.h>
#include <string.h>

TPlayer players[4];
TPlayer *pl_near[2], *pl_far[2];
enum { FRAME_TICKS = 0x2f3, MOVE_X = 0x333, MOVE_Y = 0x373,
       NEAR_FRAMES = 0x39e, FAR_FRAMES = 0x88a, START_POINTS = 0xd88, START_ANIM = 0xdcb,
       RECORD_FRAMES = 10916 };

static void select_frame(TPlayer *p)
{
    unsigned table = p->side ? FAR_FRAMES : NEAR_FRAMES;
    unsigned entry = p->anim * 10 + p->frame;
    int sprite = ds_u16(table + entry * 2) + 400 * (p->id - 1);
    spr_set_def(p->spr, sprite);
}
static void restart(TPlayer *p)
{
    p->frame = p->tick = 1;
    p->anim = ds_u8(START_ANIM + p->pos);
}
static void move_frame(TPlayer *p)
{
    int top = p->side ? 65 : 160, bottom = p->side ? 145 : 280;
    spr_move_clamped(p->spr, bottom, 337, top, 20,
                     ds_s8(MOVE_Y + p->anim), ds_s8(MOVE_X + p->anim));
}
void player_set_keys(TPlayer *p, const uint8_t *bindings, Joystick *device)
{
    memset(p->keys, 0, sizeof p->keys);
    p->nkeys = 5;
    if (bindings) memcpy(p->keys + 1, bindings, 5);
    p->joy = device;
}
void player_create(TPlayer *p, int pos, int handle, int team, int *timers)
{
    p->id = team; p->spr = handle; p->pos = pos;
    unsigned point = START_POINTS + pos * 4;
    spr_create(handle, ds_u16(point), ds_u16(point + 2));
    p->timer_a = ++*timers; p->timer_b = ++*timers;
    if (!p->rec) p->rec = calloc(RECORD_FRAMES, 6);
}
void player_destroy(TPlayer *p) { free(p->rec); p->rec = NULL; }
void player_setup(TPlayer *p, int slot, int human, int side, int pos)
{
    p->pos = pos; p->human = human; p->side = side;
    p->variant = side != 0; p->cpu = 0;
    (side ? pl_far : pl_near)[slot] = p;
    restart(p); select_frame(p);
}
void player_reset(TPlayer *p)
{
    restart(p);
    unsigned point = START_POINTS + p->pos * 4;
    spr_pos(p->spr, ds_u16(point + 2), ds_u16(point));
}
void player_set_pos(TPlayer *p, int pos) { p->pos = pos; restart(p); select_frame(p); }
void player_flip(TPlayer *p) { p->side = !p->side; p->variant = 1 - p->variant; }
void player_hide(TPlayer *p) { spr_hide(p->spr); }
void player_show(TPlayer *p) { spr_show(p->spr); }

void players_reset_all(void)
{
    for (unsigned slot = 0; slot < (g_doubles ? 2u : 1u); slot++) {
        player_reset(pl_far[slot]); player_reset(pl_near[slot]);
    }
}
void players_swap_sides(void)
{
    for (unsigned slot = 0; slot < (g_doubles ? 2u : 1u); slot++) {
        player_flip(pl_far[slot]); player_flip(pl_near[slot]);
    }
    for (unsigned slot = 0; slot < 2; slot++) {
        TPlayer *near = pl_near[slot];
        pl_near[slot] = pl_far[slot]; pl_far[slot] = near;
    }
}
void players_set_team_ids(void)
{
    for (unsigned slot = 0; slot < 2; slot++) { pl_near[slot]->id = 1; pl_far[slot]->id = 2; }
}
unsigned player_read_local(TPlayer *p)
{
    if (p->joy) return joy_bits(p->joy);
    unsigned input = 0;
    for (unsigned bit = 0; bit < p->nkeys && bit + 1 < sizeof p->keys; bit++)
        if (keys[p->keys[bit + 1]]) input |= 1u << bit;
    return input;
}
void player_poll_input(TPlayer *p)
{
    p->input = net_active() ? net_bits(p != pl_near[0]) : player_read_local(p);
}

static int locomotion(unsigned animation) { return animation >= 1 && animation <= 16; }
void player_update(TPlayer *p)
{
    int repeat = p->anim == 21 || p->anim == 24;
    int finished = p->frame >= 10;
    int accepts = finished || repeat || locomotion(p->anim);
    if (finished) {
        if (repeat) p->frame = p->tick = 1;
    } else {
        p->tick++;
        if (p->tick > ds_u8(FRAME_TICKS + p->anim)) { p->frame++; p->tick = 1; }
        move_frame(p);
    }
    if (accepts) {
        uint16_t requested;
        if (p->human) { player_poll_input(p); requested = ctrl_human(p, p->variant == 1); }
        else requested = p->variant ? ctrl_cpu_up(p) : ctrl_cpu_down(p);
        unsigned animation = requested >> 8;
        int can_restart = !locomotion(p->anim) || p->anim != animation || p->frame >= 10;
        if (can_restart) {
            p->anim = animation;
            if ((requested & 255) == 1) { p->frame = p->tick = 1; move_frame(p); }
        }
    }
    select_frame(p);
}
int player_hits_ball(TPlayer *p)
{
    Sprite *s = spr_get(p->spr);
    if (!s || p->frame < 4 || ball.alive) return 0;
    int x = g_x - s->x, y = g_y - s->y;
    int low = p->side ? -2 : -15, high = p->side ? 15 : 2;
    int ceiling = p->side ? 40 : 50, maximum = p->side ? 55 : 65;
    int overhead = p->anim >= 47 && p->anim <= 49;
    return x > 0 && x < 56 && y > low && y < high && g_h > 5 && g_h < maximum &&
           (g_h < ceiling || overhead);
}

static int16_t *record_at(int16_t *buffer, int frame)
{
    return buffer && (unsigned)frame < RECORD_FRAMES ? buffer + frame * 3 : NULL;
}
static void record_sprite(int16_t *buffer, int frame, int x, int y, int sprite)
{
    int16_t *entry = record_at(buffer, frame);
    if (entry) { entry[0] = x; entry[1] = y; entry[2] = sprite; }
}
static void restore_sprite(int16_t *buffer, int frame, int handle)
{
    int16_t *entry = record_at(buffer, frame);
    if (entry) { spr_pos(handle, entry[1], entry[0]); spr_set_def(handle, entry[2]); }
}
void player_record(TPlayer *p)
{
    Sprite *s = spr_get(p->spr);
    if (s) record_sprite(p->rec, g_rec_idx, s->x, s->y, s->id);
}
void player_replay(TPlayer *p, int frame) { restore_sprite(p->rec, frame, p->spr); }
void ball_record(TBall *b)
{
    Sprite *body = spr_get(b->spr_ball), *shadow = spr_get(b->spr_shadow);
    record_sprite(b->rec_ball, g_rec_idx, b->x, b->y - b->h, body ? body->id : 0);
    record_sprite(b->rec_shadow, g_rec_idx, b->x, b->y, shadow ? shadow->id : 0);
}
void ball_replay(TBall *b, int frame)
{
    int16_t *body = record_at(b->rec_ball, frame), *shadow = record_at(b->rec_shadow, frame);
    if (!body || !shadow) return;
    restore_sprite(b->rec_ball, frame, b->spr_ball); restore_sprite(b->rec_shadow, frame, b->spr_shadow);
    g_x = shadow[0]; g_y = shadow[1]; g_h = shadow[1] - body[1];
}
void players_apply_pos(int change_server, int change_ends)
{
    unsigned action = !change_ends ? 1 : change_server ? 3 : 2;
    unsigned table = g_doubles ? 0xdf0 : 0xdd8;
    for (unsigned slot = 0; slot < (g_doubles ? 2u : 1u); slot++) {
        TPlayer *pair[2] = {pl_far[slot], pl_near[slot]};
        for (unsigned i = 0; i < 2; i++) pair[i]->pos = ds_u8(table + pair[i]->pos * 3 + action);
    }
}
