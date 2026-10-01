#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include "joy.h"
#include "dsimg.h"
#include "sprites.h"
#include "platform.h"
#include "video.h"
#include <stdlib.h>
#include <string.h>

TPlayer players[4];
TPlayer *pl_near[2], *pl_far[2];

#define DS_ANIM_TICKS 0x2f3           /* ticks each frame of animation n lasts */
#define DS_ANIM_DY    0x373           /* per-tick movement of animation n */
#define DS_ANIM_DX    0x333
#define DS_FRAMES_D   0x39e           /* [anim*10 + frame] -> sprite id, near ("D") side */
#define DS_FRAMES_U   0x88a
#define DS_START_POS  0xd88           /* {x, y} per position index */
#define DS_START_ANIM 0xdcb

static int sprite_def(const TPlayer *p)
{
    unsigned base = p->side == 0 ? DS_FRAMES_D : DS_FRAMES_U;
    return (p->id - 1) * 400 + ds_u16(base + p->anim * 0x14 + p->frame * 2);
}

void player_set_keys(TPlayer *p, const uint8_t k[5], Joystick *j)
{
    memset(p->keys, 0, sizeof p->keys);
    p->nkeys = 5;
    if (k) for (int i = 0; i < 5; i++) p->keys[i + 1] = k[i];
    p->joy = j;
}

void player_create(TPlayer *p, int pos, int handle, int id, int *timer_ctr)
{
    p->id = id;
    p->spr = handle;
    p->pos = pos;
    spr_create(handle, ds_u16(DS_START_POS + pos * 4), ds_u16(DS_START_POS + pos * 4 + 2));
    p->timer_a = ++*timer_ctr;
    p->timer_b = ++*timer_ctr;
    p->rec = malloc(0xffd8);
}

void player_destroy(TPlayer *p) { free(p->rec); p->rec = NULL; }

void player_setup(TPlayer *p, int slot, int human, int side, int pos)
{
    p->frame = 1;
    p->tick = 1;
    p->pos = pos;
    p->cpu = 0;
    p->anim = ds_u8(DS_START_ANIM + p->pos);
    p->human = human;
    p->side = side;
    if (side == 0) { p->variant = 0; pl_near[slot] = p; }
    else           { p->variant = 1; pl_far[slot] = p; }
    spr_set_def(p->spr, sprite_def(p));
}

void player_reset(TPlayer *p)
{
    spr_pos(p->spr, ds_u16(DS_START_POS + p->pos * 4 + 2), ds_u16(DS_START_POS + p->pos * 4));
    p->anim = ds_u8(DS_START_ANIM + p->pos);
    p->frame = 1;
    p->tick = 1;
}

void player_set_pos(TPlayer *p, int pos)
{
    p->pos = pos;
    p->frame = 1;
    p->tick = 1;
    p->anim = ds_u8(DS_START_ANIM + p->pos);
    spr_set_def(p->spr, sprite_def(p));
}

void player_flip(TPlayer *p)
{
    p->side = !p->side;
    p->variant = 1 - p->variant;
}

void player_hide(TPlayer *p) { spr_hide(p->spr); }
void player_show(TPlayer *p) { spr_show(p->spr); }

void players_reset_all(void)
{
    player_reset(pl_far[0]);
    player_reset(pl_near[0]);
    if (g_doubles) { player_reset(pl_far[1]); player_reset(pl_near[1]); }
}

/* 1000:6437 - the two teams change ends */
void players_swap_sides(void)
{
    player_flip(pl_far[0]);
    player_flip(pl_near[0]);
    if (g_doubles) { player_flip(pl_far[1]); player_flip(pl_near[1]); }
    TPlayer *t;
    t = pl_far[0]; pl_far[0] = pl_near[0]; pl_near[0] = t;
    t = pl_far[1]; pl_far[1] = pl_near[1]; pl_near[1] = t;
}

void players_set_team_ids(void)
{
    pl_near[0]->id = 1; pl_near[1]->id = 1;
    pl_far[0]->id = 2;  pl_far[1]->id = 2;
}

/* keyboard/joystick -> control bits (1000:3f85) */
void player_poll_input(TPlayer *p)
{
    unsigned bits = 0;
    if (!p->joy) {
        for (int i = 1; i <= p->nkeys; i++)
            if (keys[p->keys[i]]) bits |= 1u << (i - 1);
    }
    else {
        bits = joy_bits(p->joy);
        if (bits && getenv("TT_DEBUG")) fprintf(stderr, "joy bits %02x\n", bits);
    }
    p->input = bits;
}

/* 1000:3b15 - advance the animation, move the sprite, and let the controller pick the next animation */
void player_update(TPlayer *p)
{
    int can_command;
    if (p->frame < 10) {
        p->tick++;
        if (ds_u8(DS_ANIM_TICKS + p->anim) < p->tick) {
            p->frame++;
            p->tick = 1;
        }
        spr_move_clamped(p->spr,
                         p->side == 0 ? 300 - 0x14 : 0x91, 416 - 0x4f,
                         p->side == 0 ? 0xa0 : 0x41, 0x14,
                         ds_s8(DS_ANIM_DY + p->anim), ds_s8(DS_ANIM_DX + p->anim));
        can_command = p->anim == 0x18 || p->anim == 0x15 || (p->anim != 0 && p->anim < 0x11);
    } else {
        if (p->anim == 0x18 || p->anim == 0x15) { p->frame = 1; p->tick = 1; }
        can_command = 1;
    }

    if (can_command) {
        uint16_t r;
        if (!p->human) {
            r = p->variant == 0 ? ctrl_cpu_down(p) : ctrl_cpu_up(p);
        } else {
            player_poll_input(p);
            r = ctrl_human(p, p->variant == 1);
        }
        uint8_t na = r >> 8, flag = r & 0xff;
        if ((p->anim == 0 || p->anim > 0x10 || p->anim != na || p->frame > 9)) {
            p->anim = na;
            if (flag == 1) {
                p->frame = 1;
                p->tick = 1;
                spr_move_clamped(p->spr,
                                 p->side == 0 ? 300 - 0x14 : 0x91, 416 - 0x4f,
                                 p->side == 0 ? 0xa0 : 0x41, 0x14,
                                 ds_s8(DS_ANIM_DY + p->anim), ds_s8(DS_ANIM_DX + p->anim));
            }
        }
    }
    spr_set_def(p->spr, sprite_def(p));
}

/* 1000:3e1f - is the ball within reach of this player's racket? */
int player_hits_ball(TPlayer *p)
{
    if (p->frame < 4) return 0;
    Sprite *s = spr_get(p->spr);
    if (!s || ball.alive) return 0;
    int x = s->x, y = s->y;
    int overhead = p->anim == 0x2f || p->anim == 0x30 || p->anim == 0x31;
    if (p->side == 0)
        return g_y < y + 2 && y - 0xf < g_y && x < g_x && g_x < x + 0x38 && g_h > 5 && g_h < 0x41 && (g_h < 0x32 || overhead);
    return y - 2 < g_y && g_y < y + 0xf && x < g_x && g_x < x + 0x38 && g_h > 5 && g_h < 0x37 && (g_h < 0x28 || overhead);
}

/* replay buffers hold {x, y, sprite id} per frame (1000:3804 / 3870) */
void player_record(TPlayer *p)
{
    Sprite *s = spr_get(p->spr);
    if (!s) return;
    int16_t *e = p->rec + g_rec_idx * 3;
    e[0] = s->x; e[1] = s->y; e[2] = s->id;
}

void player_replay(TPlayer *p, int idx)
{
    int16_t *e = p->rec + idx * 3;
    spr_pos(p->spr, e[1], e[0]);
    spr_set_def(p->spr, e[2]);
}

void ball_record(TBall *b)
{
    Sprite *sb = spr_get(b->spr_ball), *ss = spr_get(b->spr_shadow);
    int16_t *e = b->rec_ball + g_rec_idx * 3;
    e[0] = b->x; e[1] = b->y - b->h; e[2] = sb ? sb->id : 0;
    e = b->rec_shadow + g_rec_idx * 3;
    e[0] = b->x; e[1] = b->y; e[2] = ss ? ss->id : 0;
}

void ball_replay(TBall *b, int idx)
{
    int16_t *e = b->rec_ball + idx * 3;
    spr_pos(b->spr_ball, e[1], e[0]);
    spr_set_def(b->spr_ball, e[2]);
    g_h = e[1];
    e = b->rec_shadow + idx * 3;
    spr_pos(b->spr_shadow, e[1], e[0]);
    spr_set_def(b->spr_shadow, e[2]);
    g_x = e[0];
    g_y = e[1];
    g_h = e[1] - g_h;
}

/* 1000:39f7 / 3a84 - after a point every player moves to a new position index.
 * (p1, p2) = (0,0) normal, (0,1) and (1,1) the two kinds of end change; table at DS:0DD8 (DS:0DF0 doubles). */
static void apply_pos(TPlayer *p, int p1, int p2)
{
    int sel = p2 == 0 ? 1 : (p1 == 0 ? 2 : 3);
    p->pos = ds_u8((g_doubles == 0 ? 0xdd8 : 0xdf0) + p->pos * 3 + sel);
}

void players_apply_pos(int p1, int p2)
{
    apply_pos(pl_far[0], p1, p2);
    apply_pos(pl_near[0], p1, p2);
    if (g_doubles) { apply_pos(pl_far[1], p1, p2); apply_pos(pl_near[1], p1, p2); }
}
