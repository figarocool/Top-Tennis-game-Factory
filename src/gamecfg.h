#ifndef GAMECFG_H
#define GAMECFG_H
/* Settings normally chosen in the menus (stored in TENNIS.OPT in the original). */
typedef struct {
    int doubles;        /* 0 singles, 1 doubles */
    int speed;          /* 1 slow (45 fps), 2 normal (63/70), 3 fast (100) */
    int best_of_3;
    int court;          /* 1 clay, 2 hard, 3 grass, 4 indoor */
    int cpu_level;      /* 1 beginner .. 4 top */
    int p1_control;     /* 1 keyboard 1, 2 keyboard 2, 3 joystick 1, 4 joystick 2, 5 computer */
    int p2_control;
} GameConfig;

extern GameConfig cfg;
int  game_boot(const char *dir);
void game_make_sprites(void);
void court_select(int type);
#endif
