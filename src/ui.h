#ifndef UI_H
#define UI_H
#include "menu.h"

/* All menus of the game, built as in 1000:a84d. Item indices below are 1-based like FUN_1010_05a6(menu, n). */
extern Menu menu_main, menu_play, menu_tour, menu_season, menu_machine, menu_training, menu_options;

/* settings stored in the menus' choice items (the original keeps them there too) */
enum { PLAY_FRIENDLY = 1, PLAY_TOURNAMENT, PLAY_SEASON, PLAY_NUMBER, PLAY_CPU, PLAY_LENGTH, PLAY_COURT, PLAY_REPLAY, PLAY_HALL };
enum { OPT_MUSIC = 1, OPT_SFX, OPT_SPEED, OPT_P1, OPT_P2, OPT_P3, OPT_P4 };
enum { TRN_SERVE = 1, TRN_MACHINE, TRN_SIDE, TRN_POSITION, TRN_DELAY, TRN_COURT };

void ui_build(void);
void ui_apply_options(void);                        /* sound volume from the OPTIONS menu */
int  menu_choice(Menu *m, int idx1);                /* selected choice (1-based), FUN_1010_1b7f(05a6(m,idx)) */
void menu_set_choice(Menu *m, int idx1, int sel1);  /* FUN_1010_1b9e */
#endif
