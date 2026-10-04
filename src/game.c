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
#include "original.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* User data contains 34 animation filenames per colour; sprite IDs use
 * ten slots per animation. Frame counts are discovered from the archive. */
static int asset_exists(const char *name)
{
    for (int i = 0; i < pak_count(); i++)
        if (!strcasecmp(name, pak_name(i))) return 1;
    return 0;
}

static void load_sprite_assets(void)
{
    unsigned offset = 0x32e4;
    for (unsigned family = 0; family < 68; family++) {
        char name[96];
        snprintf(name, sizeof name, "%s", ds_cstr(offset));
        offset += (unsigned)strlen(name) + 1;
        char *frame = strrchr(name, '.');
        if (!frame || frame == name) continue;
        unsigned base = (family % 34 + 1) * 10 + (family / 34) * 400;
        for (unsigned i = 0; i < 10; i++) {
            frame[-1] = (char)('0' + i);
            if (asset_exists(name)) spr_load_def(name, base + i);
        }
    }
    spr_load_def(ds_cstr(0x31ca), 2); /* shadow */
    spr_load_def(ds_cstr(0x31e2), 3); /* out sign */
    spr_load_def(ds_cstr(0x31f7), 4); /* net */
    spr_load_def(ds_cstr(0x320f), 5); /* ball machine */
    spr_load_def(ds_cstr(0x3229), 6);
    char name[96];
    snprintf(name, sizeof name, "%s", ds_cstr(0x3243));
    char *frame = strrchr(name, '.');
    for (int i = 0; i < 7 && frame && frame > name; i++) {
        frame[-1] = (char)('1' + i);
        spr_load_def(name, 790 + i);
    }
}

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
    if (ia) blit(bg->px, VW, VH, ia, 0, 0);
    if (ib) blit(bg->px, VW, VH, ib, 0, ia ? ia->h : 0);
    spr_set_background(bg);
    img_free(bg); img_free(ia); img_free(ib);
    video_set_palette(&court_pal[type]);
    g_court_type = type;
}

int game_boot(const char *dir)
{
    char p[1024];
    if (original_path(p, sizeof p, dir, "TENNIS.DAT") || pak_open(p)) {
        fprintf(stderr, "TENNIS.DAT missing or invalid in %s\n", dir);
        notice_missing_data(dir, "TENNIS.DAT MISSING OR INVALID");
        return -1;
    }
    int status = ds_load_dat(p);
    if (status == 1) {
        if (original_path(p, sizeof p, dir, "TENNIS.EXE")) {
            fprintf(stderr, "Provide TENNIS.EXE or use tools/prepare_data.py to prepare your TENNIS.DAT in %s\n", dir);
            notice_missing_data(dir, "TENNIS.EXE OR PREPARED DAT REQUIRED");
            pak_close();
            return -1;
        }
        status = ds_load(p);
    }
    if (status < 0) {
        fprintf(stderr, "%s (%s)\n", ds_error(), dir);
        notice_missing_data(dir, ds_error());
        pak_close();
        return -1;
    }
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
    load_sprite_assets();
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
