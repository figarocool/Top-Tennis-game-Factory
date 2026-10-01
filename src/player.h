#ifndef PLAYER_H
#define PLAYER_H
#include <stdint.h>
#include "game.h"

/* Joystick state (unit 1020: 0c4e detect, 0cc1 calibrate-read, 0ea8.. buttons) */
typedef struct Joystick Joystick;

/* TPlayer, 40 bytes in the original (DS:A898, A8C0, A8E8, A910). */
typedef struct TPlayer {
    uint8_t  nkeys;                 /* +0x00 */
    uint8_t  keys[16];              /* +0x01 scancodes: bit i of `input` <- keys[i+1] */
    Joystick *joy;                  /* +0x11 non-NULL = joystick controlled */
    uint16_t input;                 /* +0x15 bit0 left, 1 right, 2 up, 3 down, 4 button 1, 5 button 2 */
    uint8_t  id;                    /* +0x17 team 1/2 (selects white/blue sprite set) */
    uint8_t  spr;                   /* +0x18 sprite handle */
    uint8_t  anim;                  /* +0x19 current animation */
    uint8_t  frame;                 /* +0x1a frame inside the animation (1-based) */
    uint8_t  tick;                  /* +0x1b ticks spent in the frame */
    uint8_t  pos;                   /* +0x1c position/situation index (start positions, DS:0D88) */
    uint8_t  variant;               /* +0x1d 0 = sprites of the near ("D") side, 1 = far ("U") side */
    uint8_t  cpu;                   /* +0x1e CPU decision flags (bit0/1 left/right, bit2/3 up/down) */
    uint8_t  side;                  /* +0x1f 0 = near side, 1 = far side */
    uint8_t  human;                 /* +0x20 */
    uint8_t  timer_a, timer_b;      /* +0x21 +0x22 timer ids */
    int16_t *rec;                   /* +0x23 replay buffer */
} TPlayer;

/* player slots: [0]=DS:9B24 [1]=DS:9B28 are the near-side players, [2]=9B1C [3]=9B20 the far side */
extern TPlayer players[4];
extern TPlayer *pl_near[2];         /* DS:9B24, 9B28 */
extern TPlayer *pl_far[2];          /* DS:9B1C, 9B20 */

uint16_t ctrl_human(TPlayer *p, int up);        /* 408b / 486a */
uint16_t ctrl_cpu_down(TPlayer *p);             /* 5049 */
uint16_t ctrl_cpu_up(TPlayer *p);               /* 5a42 */
#endif

/* timer ids are handed out sequentially at player creation (DS:9B52) */
void player_set_keys(TPlayer *p, const uint8_t k[5], Joystick *j);              /* 1000:392f */
void player_create(TPlayer *p, int pos, int handle, int id, int *timer_ctr);    /* 1000:3431 */
void player_setup(TPlayer *p, int slot, int human, int side, int pos);          /* 1000:34d3 */
void player_destroy(TPlayer *p);                                                /* 1000:34b3 */
void player_reset(TPlayer *p);                                                  /* 1000:362d */
void player_set_pos(TPlayer *p, int pos);                                       /* 1000:36d1 */
void player_flip(TPlayer *p);                                                   /* 1000:3795 */
void player_update(TPlayer *p);                                                 /* 1000:3b15 */
unsigned player_read_local(TPlayer *p);                                        /* buttons of this player's own keys / joystick */
void player_poll_input(TPlayer *p);                                             /* 1000:3f85 */
int  player_hits_ball(TPlayer *p);                                              /* 1000:3e1f */
void player_record(TPlayer *p);                                                 /* 1000:3804 */
void player_replay(TPlayer *p, int idx);                                        /* 1000:3870 */
void player_hide(TPlayer *p);                                                   /* 1000:37ce */
void player_show(TPlayer *p);                                                   /* 1000:37e9 */
void players_reset_all(void);                                                   /* 1000:3692 */
void players_swap_sides(void);                                                  /* 1000:6437 */
void players_set_team_ids(void);                                                /* 1000:3ae5 */
void players_apply_pos(int p1, int p2);                                         /* 1000:3a84 / 39f7 */

/* shots (1000:259c, 3126) */
void ball_shot(TBall *b, uint8_t anim);
void shot_sound(uint8_t anim);

/* ball replay (1000:235c / 2421) */
void ball_record(TBall *b);
void ball_replay(TBall *b, int idx);
