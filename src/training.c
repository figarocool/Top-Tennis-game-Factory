/* Training modes: serve training (1000:75e3) and ball machine training (1000:7aff), set up by b57b / b82d. */
#include "gamecfg.h"
#include "game.h"
#include "player.h"
#include "sprites.h"
#include "video.h"
#include "match.h"
#include "score.h"
#include "menu.h"
#include "ui.h"
#include "options.h"
#include "dialog.h"
#include "text.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

enum { H_OUT = 10, H_SHADOW = 5, H_BALL = 6, H_MACH_U = 8, H_MACH_D = 9 };

static void ball_show(void) { spr_show(H_BALL); spr_show(H_SHADOW); }
static void ball_hide(void) { spr_hide(H_BALL); spr_hide(H_SHADOW); }
static int esc_down(void) { plat_poll(); return keys[1] || quit_requested; }

/* 1000:75e3 - serve over and over until ESC. rotate: change position after each serve. */
static void serve_training(int rotate, int side)
{
    ball_hide();
    g_quit_match = 0;
    TPlayer *pl = side == 0 ? pl_near[0] : pl_far[0];
    do {
        spr_hide(H_OUT);
        if (side == 0) { player_reset(pl_near[0]); serve_near(1); }
        else           { player_reset(pl_far[0]);  serve_far(1); }
        if (!g_quit_match) {
            g_rally = 1;
            int out = 0;
            do {
                timer_start(g_frame_timer);
                if (!ball_step(&ball, 1)) break;
                camera_update();
                ball_net(&ball);
                if (!out && ball.bounces == 1) out = ball_out(&ball); 
                if (out) spr_show(H_OUT);
                player_update(pl);
                g_rally = g_rally && ball.bounces == 0;
                pause_check(); boss_check();
                frame_wait();
            } while (!esc_down());
        }
        if (rotate) players_apply_pos(0, 0);
        spr_hide(H_OUT);
    } while (!esc_down());
    ball_hide();
}

/* 1000:774f / 7924 - the machine throws shot A..L from the far (side 0) or near (side 1) end */
static void machine_throw(int letter, int side)
{
    ball.dir = side == 0 ? 0 : 1;
    if (letter == '?') letter = 'A' + rnd(12);
    if (side == 0) {
        switch (letter) {
        case 'A': ball_throw(&ball, 2, 0x12, 0x28, 0xd2, 0xd4, 0x5d, 0xd4); break;
        case 'B': ball_throw(&ball, 2, 0x12, 0x28, 0xd2, 0x6e, 0x5d, 0xd4); break;
        case 'C': ball_throw(&ball, 2, 0x12, 0x28, 0xd2, 0x136, 0x5d, 0xd4); break;
        case 'D': ball_throw(&ball, 3, 0x12, 0x28, 0xd2, 0xd4, 0x5d, 0xd4); break;
        case 'E': ball_throw(&ball, 3, 0x12, 0x28, 0xd2, 0x6e, 0x5d, 0xd4); break;
        case 'F': ball_throw(&ball, 3, 0x12, 0x28, 0xd2, 0x136, 0x5d, 0xd4); break;
        case 'G': ball_throw(&ball, 2, 0x12, 0x28, 0xaa, 0xd4, 0x5d, 0xd4); break;
        case 'H': ball_throw(&ball, 2, 0x12, 0x28, 0xaa, 0x82, 0x5d, 0xd4); break;
        case 'I': ball_throw(&ball, 2, 0x12, 0x28, 0xaa, 0x122, 0x5d, 0xd4); break;
        case 'J': ball_throw(&ball, 2, 0x12, 0x52, 0xd2, 0xd4, 0x5d, 0xd4); break;
        case 'K': ball_throw(&ball, 2, 0x12, 0x52, 0xd2, 0x6e, 0x5d, 0xd4); break;
        case 'L': ball_throw(&ball, 2, 0x12, 0x52, 0xd2, 0x136, 0x5d, 0xd4); break;
        default:  ball_throw(&ball, 2, 0x12, 0x28, 0xd2, 0xd4, 0x5d, 0xd4); break;
        }
    } else {
        switch (letter) {
        case 'A': ball_throw(&ball, 2, 0, 0x28, 0x6e, 0xd2, 0xe6, 200); break;
        case 'B': ball_throw(&ball, 2, 0, 0x28, 0x6e, 0x8c, 0xe6, 200); break;
        case 'C': ball_throw(&ball, 2, 0, 0x28, 0x6e, 0x118, 0xe6, 200); break;
        case 'D': ball_throw(&ball, 3, 0, 0x28, 0x6e, 0xd2, 0xe6, 200); break;
        case 'E': ball_throw(&ball, 3, 0, 0x28, 0x6e, 0x8c, 0xe6, 200); break;
        case 'F': ball_throw(&ball, 3, 0, 0x28, 0x6e, 0x118, 0xe6, 200); break;
        case 'G': ball_throw(&ball, 2, 0, 0x28, 0x87, 0xd2, 0xe6, 200); break;
        case 'H': ball_throw(&ball, 2, 0, 0x28, 0x87, 0x8c, 0xe6, 200); break;
        case 'I': ball_throw(&ball, 2, 0, 0x28, 0x87, 0x118, 0xe6, 200); break;
        case 'J': ball_throw(&ball, 2, 0, 0x55, 0x6e, 0xd2, 0xe6, 200); break;
        case 'K': ball_throw(&ball, 2, 0, 0x55, 0x6e, 0x8c, 0xe6, 200); break;
        case 'L': ball_throw(&ball, 2, 0, 0x55, 0x6e, 0x118, 0xe6, 200); break;
        default:  ball_throw(&ball, 2, 0, 0x28, 0x6e, 0xd2, 0xe6, 200); break;
        }
    }
}

/* 1000:7aff - ball machine. delay: frames between balls; shots: the 12 selected letters */
static void machine_training(int delay, const char *shots, int side)
{
    ball_hide();
    g_quit_match = 0;
    TPlayer *pl = side == 0 ? pl_near[0] : pl_far[0];
    player_reset(pl);
    spr_show(side == 0 ? H_MACH_U : H_MACH_D);
    g_rally = 0;
    int inflight = 0, out = 0, countdown = delay, idx = 0;
    do {
        if (!inflight && --countdown == 0) {
            int len = (int)strlen(shots);
            if (len <= idx) idx = 0;
            machine_throw(shots[idx], side);
            ball_show();
            idx++;
            spr_hide(H_OUT);
            out = 0;
            inflight = 1;
            timer_start(g_frame_timer);
            snd_play(9);
        }
        if (inflight && !ball_step(&ball, 1)) { inflight = 0; countdown = delay; }
        spr_sort(1, 9, H_BALL, H_SHADOW);
        spr_render(0x32, 0x32);
        video_wait_vsync();
        if (inflight) {
            ball_net(&ball);
            out = !out && ball.bounces == 1 ? ball_out(&ball) : out;
            if (!out) {
                if (ball.bounces < 2) {
                    if (!ball.dir && side == 0) {
                        if (player_hits_ball(pl)) ball_shot(&ball, pl->anim);
                    } else if (ball.dir && side != 0) {
                        if (player_hits_ball(pl)) ball_shot(&ball, pl->anim);
                    }
                }
            } else spr_show(H_OUT);
        }
        player_update(pl);
        pause_check(); boss_check();
        if (inflight) while (timer_elapsed(g_frame_timer) < g_frame_period) plat_poll();
    } while (!esc_down());
    spr_hide(H_OUT);
    ball_hide();
    spr_hide(side == 0 ? H_MACH_U : H_MACH_D);
}

/* common setup (b57b / b82d): one human player on the chosen side, training court */
static int training_setup(int machine, int rotate_unused)
{
    (void)rotate_unused;
    OptFile o;
    opt_from_menus(&o);
    int ctl = o.o[24];
    int side_up = o.o[17];                    /* 1 = player side UP (far), 0 = DOWN (near) */
    if (ctl == 3 || ctl == 4) {
        Dialog d;
        dialog_init(&d, "          SORRY...", 5, 0x1f, 0x60, 0x28);
        dialog_add_label(&d, ctl == 3 ? "JOYSTICK 1 NOT DETECTED!" : "JOYSTICK 2 NOT DETECTED!", 0x73, 0x3f);
        dialog_add_button(&d, " @OK ", 1, 0x8a, 0x84);
        font_select(8);
        dialog_run(&d);
        return 0;
    }
    video_fade_out(2);
    game_make_sprites();
    g_doubles = 0;
    score_init(&score);
    score_set_names(&score, "ANDREW SEAGAL", "PETER PRESSMAN");
    for (int i = 0; i < 4; i++) players[i].human = 0;
    TPlayer *me = side_up ? pl_far[0] : pl_near[0];
    me->human = 1;
    player_set_pos(me, side_up ? (machine ? 8 : 4) : (machine ? 5 : 1));
    player_set_keys(me, ctl == 5 ? NULL : opt.keys[ctl == 2], NULL);
    players_reset_all();
    court_select(o.o[20]);
    if (!side_up) { player_hide(pl_far[0]); player_show(pl_near[0]); } else { player_show(pl_far[0]); player_hide(pl_near[0]); }
    player_hide(pl_far[1]); player_hide(pl_near[1]);
    g_rally = 0;
    int fps = o.o[23] == 1 ? 45 : o.o[23] == 3 ? 100 : 63;
    match_set_speed(fps);
    Palette court = vpal;
    video_black();
    video_set_split(200);
    spr_render(0, 0);
    video_fade_in(&court, 2);
    return side_up ? 2 : 1;
}

int act_serve_training(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    OptFile o; opt_from_menus(&o);
    int side = training_setup(0, 0);
    if (!side) return MR_STAY;
    int rotate = o.o[18] == 0;                /* ROTATIVE */
    serve_training(rotate, side == 2);
    video_set_scroll(0, 0);
    video_fade_out(2);
    ui_dirty = 1;
    return MR_STAY;
}

int act_machine_continue(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    OptFile o; opt_from_menus(&o);
    int side = training_setup(1, 0);
    if (!side) return MR_STAY;
    char shots[13];
    memcpy(shots, o.o + 4, 12); shots[12] = 0;
    machine_training(o.o[19] * 25, shots, side == 2);
    video_set_scroll(0, 0);
    video_fade_out(2);
    ui_dirty = 1;
    return MR_STAY;
}
