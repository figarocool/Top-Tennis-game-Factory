/* Game flows started from the menus: friendly match (1000:c28e) and the helpers it uses. */
#include "gamecfg.h"
#include "game.h"
#include "player.h"
#include "sprites.h"
#include "video.h"
#include "text.h"
#include "hud.h"
#include "match.h"
#include "score.h"
#include "menu.h"
#include "ui.h"
#include "dialog.h"
#include "options.h"
#include "platform.h"
#include "screens.h"
#include "matchflow.h"
#include "joy.h"
#include "sound.h"
#include "hof.h"
#include "net.h"
#include "dsimg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* control types in the OPTIONS menu: 1,2 keyboard sets, 3,4 joysticks, 5 computer */

int message_box(const char *title, const char *line1, const char *line2)
{
    Dialog d;
    dialog_init(&d, title, 5, 0x1c, 0x60, 0x32);
    dialog_add_label(&d, line1, 0x73, 0x3f);
    if (line2) dialog_add_label(&d, line2, 0x7d, 0x3f);
    dialog_add_button(&d, ds_cstr(0x3057), 1, 0x8a, 0x7d);
    font_select(8);
    return dialog_run(&d);
}

/* 1000:dfdf - ask for the human players' names. Returns 0 when cancelled. */
int ask_names(int p1_human, int p2_human, char *n1, char *n2)
{
    char b1[24] = "", b2[24] = "";
    font_select(8);
    if (!p1_human) snprintf(n1, 24, "%s", ds_player_name(0));
    else {
        Dialog d;
        dialog_init(&d, ds_cstr(0x30f6), 5, 0x1c, 0x60, 0x32);
        dialog_add_edit(&d, b1, 20, 20, 0x77, 0x50);
        dialog_add_button(&d, ds_cstr(0x3057), 1, 0x8a, 0x46);
        dialog_add_button(&d, ds_cstr(0x1c2c), 0, 0x8a, 0xbe);
        if (!dialog_run(&d)) return 0;
        for (char *c = b1; *c; c++) *c = (char)toupper((unsigned char)*c);       /* FUN_1010_3e7b: StrUpper */
        snprintf(n1, 24, "%s", b1);
    }
    if (!p2_human) {
        snprintf(n2, 24, "%s", ds_player_name(p1_human ? 0 : 1));
    } else {
        for (;;) {
            Dialog d;
            b2[0] = 0;
            dialog_init(&d, ds_cstr(0x311a), 5, 0x1c, 0x60, 0x32);
            dialog_add_edit(&d, b2, 20, 20, 0x77, 0x50);
            dialog_add_button(&d, ds_cstr(0x3057), 1, 0x8a, 0x46);
            dialog_add_button(&d, ds_cstr(0x1c2c), 0, 0x8a, 0xbe);
            if (!dialog_run(&d)) return 0;
            for (char *c = b2; *c; c++) *c = (char)toupper((unsigned char)*c);
            if (!strstr(b2, n1)) break;
            message_box("  ", ds_cstr(0x3090), NULL);
        }
        snprintf(n2, 24, "%s", b2);
    }
    return 1;
}

/* 1000:a187 / a229 - joystick availability; the original shows a notice when the joystick is missing */
static int joystick_available(int type)
{
    return joy_open(type == CTRL_JOY1 ? 1 : 2) != NULL;
}

int device_check(int type)
{
    if (type == CTRL_JOY1 || type == CTRL_JOY2) {
        if (!joystick_available(type)) {
            message_box(ds_cstr(0x30aa), type == CTRL_JOY1 ? ds_cstr(0x1a0e) : ds_cstr(0x1a27), NULL);
            return 0;
        }
    }
    return 1;
}

static void setup_keys(TPlayer *p, int type)
{
    p->joy = NULL;
    if (type == CTRL_NET) { p->nkeys = 0; return; }
    if (type == CTRL_KB1 || type == CTRL_KB2) player_set_keys(p, opt.keys[type == CTRL_KB2], NULL);
    else if (type == CTRL_JOY1 || type == CTRL_JOY2) { p->nkeys = 0; p->joy = joy_open(type == CTRL_JOY1 ? 1 : 2); }
}

int matchflow_play(const MatchSetup *ms, int fade_first)
{
    ui_apply_options();
    if (fade_first) video_fade_out(2);
    game_make_sprites();
    g_doubles = ms->doubles;
    score_init(&score);
    score_set_names(&score, ms->name1, ms->name2);
    score_set_best_of_3(&score, ms->best_of_3);

    TPlayer *ps[4] = { pl_near[0], pl_far[0], pl_near[1], pl_far[1] };
    static const int cpu_flags[5] = { 0, 0, 5, 6, 10 };
    static const int startpos[4] = { 1, 8, 10, 0xf };
    for (int i = 0; i < 4; i++) {
        ps[i]->human = ms->human[i];
        ps[i]->cpu = cpu_flags[ms->cpu_level[i] > 4 ? 4 : ms->cpu_level[i]];
        player_set_pos(ps[i], startpos[i]);
        setup_keys(ps[i], ms->ctrl[i]);
    }
    players_reset_all();
    court_select(ms->court);
    for (int i = 0; i < 4; i++) player_show(&players[i]);
    if (!g_doubles) { player_hide(pl_far[1]); player_hide(pl_near[1]); }
    int fps = ms->speed == 1 ? 45 : ms->speed == 3 ? 100 : 63 + 7 * g_doubles;
    match_set_speed(fps);

    Palette court = vpal;
    video_black();
    video_set_split(200);
    g_rec_idx = 0;
    spr_render(0, 0);
    video_fade_in(&court, 2);
    music_stop();
    if (ms->net) {
        extern void netplay_prepare(void);
        netplay_prepare();                       /* local reader + state hash, then both machines start counting frames */
        net_begin_match();
    }
    if (getenv("TT_SWAP")) { /* test: far side serves first, including the doubles partner */
        players_apply_pos(0, 1);
        if (!g_doubles) players_apply_pos(0, 0);
        score.server_flag ^= 1;
        players_reset_all();
    }
    int winner = play_match();
    video_set_scroll(0, 0);
    music_start(1);
    return winner;
}

void match_defaults_from_options(MatchSetup *ms)
{
    OptFile o;
    opt_from_menus(&o);
    memset(ms, 0, sizeof *ms);
    ms->doubles = o.o[0] == 0;
    ms->best_of_3 = o.o[2];
    ms->court = o.o[3];
    ms->speed = o.o[23];
    for (int i = 0; i < 4; i++) {
        ms->ctrl[i] = o.o[24 + i];
        ms->human[i] = ms->ctrl[i] != CTRL_CPU;
        ms->cpu_level[i] = o.o[1];
    }
}

int act_friendly(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    MatchSetup ms;
    match_defaults_from_options(&ms);
    if (!ask_names(ms.human[0], ms.human[1], ms.name1, ms.name2)) return MR_STAY;
    for (int i = 0; i < 4; i++) if (!device_check(ms.ctrl[i])) return MR_STAY;
    matchflow_play(&ms, 1);
    if (!g_quit_match) hof_offer_save(ds_cstr(0x1c82), " ");
    ui_dirty = 1;
    video_fade_out(2);
    return MR_STAY;
}

/* 1000:c054 - DEMO: four computer players at the top level */
int act_demo(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    MatchSetup ms;
    match_defaults_from_options(&ms);
    ms.doubles = 0;
    for (int i = 0; i < 4; i++) { ms.human[i] = 0; ms.ctrl[i] = CTRL_CPU; ms.cpu_level[i] = 4; }
    snprintf(ms.name1, sizeof ms.name1, "%s", ds_player_name(0));
    snprintf(ms.name2, sizeof ms.name2, "%s", ds_player_name(1));
    matchflow_play(&ms, 1);
    ui_dirty = 1;
    video_fade_out(2);
    return MR_STAY;
}
