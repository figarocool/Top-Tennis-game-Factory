#include "hud.h"
#include "text.h"
#include "video.h"
#include "pak.h"
#include <stdio.h>
#include <string.h>

Image *img_marcador, *img_splitmsg;

int hud_init(void)
{
    img_marcador = img_load_pbm("DATA\\BACKGND\\MARCADOR.PBM");
    img_splitmsg = img_load_pbm("DATA\\BACKGND\\SPLITMSG.PBM");
    return img_marcador && img_splitmsg ? 0 : -1;
}

/* Name as shown on the scoreboard (1000:0ae9): "A. SURNAME" when the name has two words, else the name itself.
 * 1008:07fc is true when the name has nothing but spaces after its first space. */
static int single_word(const char *n)
{
    const char *sp = strchr(n, ' ');
    if (!sp) return 1;
    for (; *sp; sp++) if (*sp != ' ') return 0;
    return 1;
}

static void board_name(const char *n, char *out)       /* out: at least 16 bytes */
{
    if (single_word(n)) {
        strncpy(out, n, 14); out[14] = 0;
        return;
    }
    out[0] = n[0]; out[1] = '.'; out[2] = ' '; out[3] = 0;
    const char *sp = strchr(n, ' ');
    const char *sur = sp ? sp + 1 : n;
    size_t room = 14 - strlen(out);
    strncat(out, sur, room);
}

/* point text: blank / " 0" / "40" / " A" / Str(n:2) as in the original string constants of segment 1000 */
static const char *point_text(const TScore *s, int p, char *buf)
{
    int a = s->pts[p], b = s->pts[1 - p];
    if (a == 0) return b == 0 ? " " : " 0";
    if (a == 40) return b == 50 ? " " : "40";
    if (a == 50) return " A";
    snprintf(buf, 8, "%2d", a);
    return buf;
}

/* 1000:0ae9 with the arguments the match code always passes: rows y=4/14, name x=11, points x=155,
 * serve marker x=141, games per set at x=190,213,236,259,282, colour 14, drawn into the status bar. */
void hud_draw_score(const TScore *s)
{
    const int Y1 = 4, Y2 = 14, XN = 11, XP = 155, COL = 14;
    const int xs[5] = { 190, 213, 236, 259, 282 };
    char buf[32], nb[24];

    board_name(s->name[0], nb); text_at(nb, COL, DST_HUD, Y1, XN);
    board_name(s->name[1], nb); text_at(nb, COL, DST_HUD, Y2, XN);
    text_at(point_text(s, 0, buf), COL, DST_HUD, Y1, XP);
    text_at(point_text(s, 1, buf), COL, DST_HUD, Y2, XP);
    text_at("*", COL, DST_HUD, s->server_flag ? Y1 : Y2, XP - 14);
    for (int i = 0; i < 5; i++) {
        if (i >= 3 && s->best_of_3) break;
        snprintf(buf, sizeof buf, "%2d", s->games[0][i]);
        text_at(buf, COL, DST_HUD, Y1, xs[i]);
        snprintf(buf, sizeof buf, "%2d", s->games[1][i]);
        text_at(buf, COL, DST_HUD, Y2, xs[i]);
    }
}

void hud_show_scoreboard(const TScore *s)
{
    dst_image(DST_HUD, img_marcador, 0, 0);
    font_select(6);
    hud_draw_score(s);
}
