#ifndef REPLAY_H
#define REPLAY_H
#include "menu.h"

void replay_play(void);                           /* 1000:7dac - F3 during a match / after loading */
int  act_load_replay(Menu *m, MenuItem *it);      /* 1000:c889 */
#endif
