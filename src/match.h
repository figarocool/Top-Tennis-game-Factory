#ifndef MATCH_H
#define MATCH_H
#include <stdint.h>

extern uint8_t  g_quit_match;        /* DS:9B68 */
extern uint8_t  g_replay_off;        /* DS:9B67 */
extern uint16_t g_rec_count;         /* DS:9B64 last frame to record */
extern int      g_frame_timer;       /* DS:9B54 timer id used for frame pacing */
extern uint32_t g_frame_period;      /* DS:9B5A PIT ticks per game frame */
extern int      g_fps;               /* DS:9B55 */

void match_set_speed(int fps);                 /* frame period = 1193200 / fps */
void camera_update(void);                      /* 1000:669c */
void frame_wait(void);                         /* busy wait until the frame period elapsed */
void pause_check(void);                        /* 1000:7f11 */
void boss_check(void);                         /* 1000:8033 */
int  quit_check(int immediate);                /* 1000:7f57 */
void show_message(int sound, const char *msg); /* 1000:7478 */
void serve_near(int practice);                 /* 1000:681c */
void serve_far(int practice);                  /* 1000:6a85 */
int  play_point(void);                         /* 1000:6cf0 */
int  play_game(void);                          /* 1000:7099 */
int  play_match(void);                         /* 1000:73d8, returns winning team id */
void match_new_ball(void);                     /* 1000:1a76 + sprite handles */
#endif
