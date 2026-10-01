#ifndef TOURNAMENT_H
#define TOURNAMENT_H
#include <stdint.h>
#include "menu.h"

#define NPLAYERS 64

typedef struct { char name[20]; uint16_t points; uint8_t level; uint8_t ctrl; } DbPlayer;

extern DbPlayer db[NPLAYERS + 1];       /* DS:9B52 + i*0x18 (record 0 unused); sorted by points, rank = index */

void db_init(void);                     /* 1000:89b5 + 894a */
void db_sort(void);                     /* 1000:8842 */

int  tournament_menu_action(Menu *m, MenuItem *it);   /* a city of the tournament menu (1000:9dec) */
int  tournament_load_action(Menu *m, MenuItem *it);
int  season_load_action(Menu *m, MenuItem *it);
int  season_new_action(Menu *m, MenuItem *it);        /* 1000:ce2c */
#endif
