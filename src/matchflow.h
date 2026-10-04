#ifndef MATCHFLOW_H
#define MATCHFLOW_H
#include "score.h"

/* Everything needed to start a match (what 1000:c28e / 9907 / c054 each set up before calling 73d8). */
typedef struct {
    int  human[4];          /* near[0], far[0], near[1], far[1] controlled by a person */
    int  ctrl[4];           /* control type 1,2 keyboard sets, 3,4 joysticks, 5 computer */
    int  cpu_level[4];      /* 1..4 */
    int  doubles;
    int  best_of_3;
    int  court;             /* 1 clay, 2 hard, 3 grass, 4 indoor */
    int  speed;             /* 1 slow, 2 normal, 3 fast */
    char name1[24], name2[24];
    int  net;               /* network game: the other side is on another machine (control type CTRL_NET) */
} MatchSetup;

enum { CTRL_KB1 = 1, CTRL_KB2, CTRL_JOY1, CTRL_JOY2, CTRL_CPU, CTRL_NET };     /* control types of the OPTIONS menu, plus the remote player */

/* Plays a match (fade out, sprites, fade in, 73d8). Returns the winning team (1 near, 2 far); g_quit_match tells if it was abandoned. */
int matchflow_play(const MatchSetup *ms, int fade_out_first);
void match_defaults_from_options(MatchSetup *ms);   /* b03f */
int  device_check(int ctrl);                        /* 1000:a229: shows the "not detected" notice; 1 = usable */
int  ask_names(int p1_human, int p2_human, char *n1, char *n2);   /* 1000:dfdf */
int  message_box(const char *title, const char *l1, const char *l2);
#endif
