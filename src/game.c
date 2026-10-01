/* Boot: assets, fonts, sprites and the objects every match needs (the program body at 1000:ed2b). */
#include "gamecfg.h"
#include "game.h"
#include "player.h"
#include "sprites.h"
#include "video.h"
#include "text.h"
#include "hud.h"
#include "match.h"
#include "score.h"
#include "pak.h"
#include "sound.h"
#include "dsimg.h"
#include "platform.h"
#include "screens.h"
#include <stdio.h>
#include <string.h>

#include "sprite_list.inc"

GameConfig cfg = { .doubles = 0, .speed = 2, .best_of_3 = 1, .court = 1, .cpu_level = 3, .p1_control = 1, .p2_control = 5 };

static Palette court_pal[5];

static int load_court_assets(void)
{
    static const char *pal[5] = { 0, "DATA\\PAL\\SORRA.PAL", "DATA\\PAL\\HARDCOUR.PAL", "DATA\\PAL\\HERBA.PAL", "DATA\\PAL\\INDOOR.PAL" };
    for (int i = 1; i <= 4; i++) if (pal_load(pal[i], &court_pal[i])) return -1;
    return 0;
}

/* 1000:a784 - choose court surface: palette + the two background halves */
void court_select(int type)
{
    if (type < 1 || type > 4) type = 1;
    char a[64], b[64];
    snprintf(a, sizeof a, "DATA\\BACKGND\\PISTA_%dA.PBM", type);
    snprintf(b, sizeof b, "DATA\\BACKGND\\PISTA_%dB.PBM", type);
    Image *ia = img_load_pbm(a), *ib = img_load_pbm(b);
    Image *bg = img_new(VW, VH, 0);
    if (ia) memcpy(bg->px, ia->px, (size_t)VW * ia->h);
    if (ib) memcpy(bg->px + (size_t)VW * ia->h, ib->px, (size_t)VW * ib->h);
    spr_set_background(bg);
    img_free(bg); img_free(ia); img_free(ib);
    video_set_palette(&court_pal[type]);
    g_court_type = type;
}

int game_boot(const char *dir)
{
    char p[512];
    snprintf(p, sizeof p, "%s/TENNIS.DAT", dir);
    if (pak_open(p)) { fprintf(stderr, "cannot open %s\n", p); notice_missing_data(dir); return -1; }
    if (ds_load(NULL)) return -1;
    if (plat_init() || video_init()) return -1;
    if (load_court_assets()) return -1;
    if (hud_init()) return -1;
    font_slot_load("DATA\\MYFONT.FNT", 5);
    font_slot_load("DATA\\SCORE.FNT", 6);
    font_slot_load("DATA\\FONT5x5.FNT", 7);
    font_slot_load("DATA\\DIALOG1.FNT", 8);
    font_select(5);
    snd_init();
    ball_tables_init();
    rnd_seed(timer_ticks() ^ 0x5eed);
    return 0;
}

/* 1000:e889 + the object creation in the program body */
void game_make_sprites(void)
{
    spr_init();
    for (unsigned i = 0; i < sizeof misc_sprites / sizeof misc_sprites[0]; i++)
        spr_load_def(misc_sprites[i].name, misc_sprites[i].id);
    for (unsigned i = 0; i < sizeof player_sprites / sizeof player_sprites[0]; i++) {
        char name[96];
        strcpy(name, player_sprites[i].name);
        char *d0 = strstr(name, "0.CBE");
        for (int f = 0; f < player_sprites[i].n && d0; f++) {
            d0[0] = (char)('0' + f);
            spr_load_def(name, player_sprites[i].id + f);
        }
    }
    int timer_ctr = 0;
    player_create(&players[0], 1, 1, 1, &timer_ctr);
    player_create(&players[1], 8, 2, 2, &timer_ctr);
    player_create(&players[2], 10, 3, 1, &timer_ctr);
    player_create(&players[3], 0xf, 4, 2, &timer_ctr);
    g_frame_timer = ++timer_ctr; g_frame_timer = ++timer_ctr;      /* 9b53, 9b54: ids 9 and 10 */

    /* ball, shadow, net, OUT sign, ball machines (handles 5,6,7,10,8,9) */
    ball_init(&ball, 5, 6);
    spr_create(5, 0xd2, 0x96); spr_create(6, 0xd2, 0x96); spr_create(7, 0x19, 0x99);
    spr_create(10, 0xb9, 0x96); spr_create(8, 0xc8, 0x5a); spr_create(9, 0xc8, 0xf5);
    spr_set_def(5, 2); spr_set_def(6, 0x316); spr_set_def(10, 3); spr_set_def(7, 4); spr_set_def(8, 5); spr_set_def(9, 6);
    spr_hide(10); spr_hide(8); spr_hide(9);

    player_setup(&players[0], 0, 1, 0, 1);
    player_setup(&players[1], 0, 0, 1, 8);
    player_setup(&players[2], 1, 0, 0, 10);
    player_setup(&players[3], 1, 0, 1, 0xf);
}
