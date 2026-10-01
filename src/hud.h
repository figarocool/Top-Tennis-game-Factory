#ifndef TT_HUD_H
#define TT_HUD_H
#include "score.h"
#include "gfx.h"

extern Image *img_marcador;     /* DATA\BACKGND\MARCADOR.PBM  (320x25 scoreboard) */
extern Image *img_splitmsg;     /* DATA\BACKGND\SPLITMSG.PBM  (320x25 message bar) */

int  hud_init(void);
void hud_draw_score(const TScore *s);        /* FUN_1000_0ae9 with the arguments used by the match code */
void hud_show_scoreboard(const TScore *s);   /* marcador + score + SCORE font */
#endif
