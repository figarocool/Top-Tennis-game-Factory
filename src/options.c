#include <stdlib.h>
#include "options.h"
#include "video.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

OptFile opt;
char data_dir[512] = "orig";

void opt_defaults(void)
{
    memset(&opt, 0, sizeof opt);
    opt.o[0] = 1; opt.o[1] = 1; opt.o[2] = 1; opt.o[3] = 1;
    for (int i = 0; i < 12; i++) opt.o[4 + i] = 'A' + i;
    opt.o[17] = 0; opt.o[18] = 1; opt.o[19] = 1; opt.o[20] = 1;
    opt.o[21] = 5; opt.o[22] = 5; opt.o[23] = 2;
    opt.o[24] = 1; opt.o[25] = 5; opt.o[26] = 5; opt.o[27] = 5;
    static const uint8_t k1[5] = { 0xc8, 0xd0, 0xcb, 0xcd, 0x39 }, k2[5] = { 0x11, 0x1f, 0x1e, 0x20, 0x2a };
    memcpy(opt.keys[0], k1, 5); memcpy(opt.keys[1], k2, 5);
}

void opt_from_menus(OptFile *o)
{
    o->o[0] = menu_choice(&menu_play, PLAY_NUMBER) == 1;
    o->o[1] = (uint8_t)menu_choice(&menu_play, PLAY_CPU);
    o->o[2] = menu_choice(&menu_play, PLAY_LENGTH) == 1;
    o->o[3] = (uint8_t)menu_choice(&menu_play, PLAY_COURT);
    for (int i = 1; i <= 12; i++) {
        int c = menu_choice(&menu_machine, i);
        o->o[3 + i] = c == 13 ? '?' : (uint8_t)(c + '@');
    }
    o->o[16] = (uint8_t)g_aspect;             /* screen format (was unused in the original file) */
    o->o[17] = menu_choice(&menu_training, TRN_SIDE) == 1;
    o->o[18] = menu_choice(&menu_training, TRN_POSITION) == 1;
    o->o[19] = (uint8_t)menu_choice(&menu_training, TRN_DELAY);
    o->o[20] = (uint8_t)menu_choice(&menu_training, TRN_COURT);
    o->o[21] = (uint8_t)menu_choice(&menu_options, OPT_MUSIC);
    o->o[22] = (uint8_t)menu_choice(&menu_options, OPT_SFX);
    o->o[23] = (uint8_t)menu_choice(&menu_options, OPT_SPEED);
    for (int i = 0; i < 4; i++) o->o[24 + i] = (uint8_t)menu_choice(&menu_options, OPT_P1 + i);
    const char *tc = getenv("TT_CTRL");               /* test hook: "p1,p2,p3,p4" control types (1..5) */
    if (tc) for (int i = 0; i < 4 && *tc; i++) { o->o[24 + i] = (uint8_t)atoi(tc); while (*tc && *tc != ',') tc++; if (*tc) tc++; }
}

void opt_to_menus(const OptFile *o)
{
    menu_set_choice(&menu_play, PLAY_NUMBER, 2 - o->o[0]);
    menu_set_choice(&menu_play, PLAY_CPU, o->o[1]);
    menu_set_choice(&menu_play, PLAY_LENGTH, 2 - o->o[2]);
    menu_set_choice(&menu_play, PLAY_COURT, o->o[3]);
    for (int i = 1; i <= 12; i++) menu_set_choice(&menu_machine, i, o->o[3 + i] == '?' ? 13 : o->o[3 + i] - '@');
    menu_set_choice(&menu_training, TRN_SIDE, 2 - o->o[17]);
    menu_set_choice(&menu_training, TRN_POSITION, 2 - o->o[18]);
    menu_set_choice(&menu_training, TRN_DELAY, o->o[19]);
    menu_set_choice(&menu_training, TRN_COURT, o->o[20]);
    menu_set_choice(&menu_options, OPT_MUSIC, o->o[21]);
    menu_set_choice(&menu_options, OPT_SFX, o->o[22]);
    menu_set_choice(&menu_options, OPT_SPEED, o->o[23]);
    for (int i = 0; i < 4; i++) menu_set_choice(&menu_options, OPT_P1 + i, o->o[24 + i]);
    if (o->o[16] == 1 || o->o[16] == 2) video_set_aspect(o->o[16]);       /* 0 = file of the original game: keep the default */
#if defined(__vita__) || defined(__PSP__)
    menu_set_choice(&menu_options, OPT_SCREEN, g_aspect);
#endif
}

static void opt_path(char *p, size_t n) { snprintf(p, n, "%s/TENNIS.OPT", data_dir); }

int opt_load(void)
{
    char p[600];
    opt_path(p, sizeof p);
    FILE *f = fopen(p, "rb");
    if (!f) return -1;
    OptFile tmp;
    size_t n = fread(&tmp, 1, sizeof tmp, f);
    fclose(f);
    if (n != sizeof tmp) return -1;
    opt = tmp;
    opt_to_menus(&opt);
    return 0;
}

int opt_save(void)
{
    char p[600];
    opt_path(p, sizeof p);
    opt_from_menus(&opt);
    FILE *f = fopen(p, "wb");
    if (!f) return -1;
    fwrite(&opt, 1, sizeof opt, f);
    fclose(f);
    return 0;
}
