/* The match: serve, rally, point, game, match loops of the original (segment 1000, 681c..73d8). */
#include "match.h"
#include "replay.h"
#include "game.h"
#include "player.h"
#include "score.h"
#include "sprites.h"
#include "video.h"
#include "text.h"
#include "hud.h"
#include "dsimg.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint8_t  g_quit_match, g_replay_off;
uint16_t g_rec_count = 0x2aa3;
int      g_frame_timer = 10;
uint32_t g_frame_period;
int      g_fps = 63;
static uint32_t g_frame_no;                  /* DS:9B56 */

enum { H_OUT = 10, H_SHADOW = 5, H_BALL = 6 };

void match_set_speed(int fps)
{
    g_fps = fps;
    g_frame_period = 1193200u / (uint32_t)fps;
}

/* 1000:669c - camera follows the ball, then draw everything and display (waits for the vertical retrace) */
void camera_update(void)
{
    int cx, cy;
    int d = 0x50 - g_x;
    if (d < 1) cx = (g_x - 0x154 < 1) ? 0x32 : g_x - 0x122;
    else cx = 0x32 - d;
    int sy = 0x4b - (g_y - g_h);
    if (sy < 1) cy = (g_y - 0xf0 < 1) ? 0x32 : g_y - 0xbe;
    else cy = 0x32 - sy;
    if (cx < 0) cx = 0; else if (cx > VW - SCR_W) cx = VW - SCR_W;           /* c6a4 = 96 */
    int ymax = (VH - split_line) - 2;                                          /* c6a6 - 2 */
    if (cy < 0) cy = 0; else if (cy > ymax) cy = ymax;

    static int dbg = -1;
    if (dbg < 0) dbg = getenv("TT_DEBUG") ? atoi(getenv("TT_DEBUG")) : 0;
    if (dbg) {
        static int n;
        fprintf(stderr, "f%d ball x=%d y=%d h=%d t=%d amp=%d b=%d dir=%d | near a%02x.%d (%d,%d) far a%02x.%d (%d,%d) | %d-%d pts %d/%d rally=%d\n",
                ++n, ball.x, ball.y, ball.h, ball.t, ball.amp, ball.bounces, ball.dir,
                pl_near[0]->anim, pl_near[0]->frame, spr[pl_near[0]->spr].x, spr[pl_near[0]->spr].y,
                pl_far[0]->anim, pl_far[0]->frame, spr[pl_far[0]->spr].x, spr[pl_far[0]->spr].y,
                score.games[0][0], score.games[1][0], score.pts[0], score.pts[1], g_rally);
    }
    if (dbg == 3) fprintf(stderr, "cam cx=%d cy=%d g_x=%d g_y=%d g_h=%d split=%d\n", cx, cy, g_x, g_y, g_h, split_line);
    if (dbg == 2) { for (int i = 1; i <= 10; i++) fprintf(stderr, " h%d:id%d(%d,%d)%s", i, spr[i].id, spr[i].x, spr[i].y, spr[i].hidden ? "H" : ""); fprintf(stderr, "\n"); }
    spr_sort(1, 9, H_BALL, H_SHADOW);
    spr_render(cy, cx);
    video_wait_vsync();

    if (!g_replay_off) {
        g_frame_no++;
        if (g_rec_idx <= g_rec_count) {
            ball_record(&ball);
            player_record(pl_far[0]);
            player_record(pl_near[0]);
            if (g_doubles) { player_record(pl_far[1]); player_record(pl_near[1]); }
            g_rec_idx++;
        }
    }
}

void frame_wait(void)
{
    while (timer_elapsed(g_frame_timer) < g_frame_period) {
        plat_poll();
        if (g_frame_period - timer_elapsed(g_frame_timer) > PIT_HZ / 500) plat_sleep_ms(1);
    }
}

static void wait_key(int sc, int down)
{
    while ((keys[sc] != 0) != down && !quit_requested) { video_wait_vsync(); }
}

void pause_check(void)
{
    if (keys[0x3f]) {                 /* F5: pause until pressed again */
        wait_key(0x3f, 0);
        wait_key(0x3f, 1);
        wait_key(0x3f, 0);
    }
}

void boss_check(void)
{
    if (keys[0x44]) {                 /* F10: boss screen */
        wait_key(0x44, 0);
        Image *boss = img_load_pbm("DATA\\BACKGND\\BOSS_SCR.PBM");
        if (boss) {
            static uint8_t save[VW * VH], savehud[VW * HUD_H];
            Palette sp = vpal; int ssx = scroll_x, ssy = scroll_y, sspl = split_line;
            memcpy(save, vpage, sizeof save); memcpy(savehud, vhud, sizeof savehud);
            blit(vpage, VW, VH, boss, 0, 0);
            video_set_scroll(0, 0); video_set_split(200);
            video_present();
            wait_key(0x44, 1);
            wait_key(0x44, 0);
            memcpy(vpage, save, sizeof save); memcpy(vhud, savehud, sizeof savehud);
            video_set_palette(&sp); video_set_scroll(ssy, ssx); video_set_split(sspl);
            img_free(boss);
        }
    }
}

/* 1000:7f57 - ESC asks "QUIT MATCH? (Y/N)" (immediate = quit without asking, used by training modes) */
int quit_check(int immediate)
{
    if (quit_requested) { g_quit_match = 1; return 1; }
    if (keys[1]) {
        if (!immediate) {
            if (split_line != 200) video_split_slide(1, 200, split_line);
            font_select(5);
            dst_image(DST_HUD, img_splitmsg, 0, 0);
            text_outlined("QUIT MATCH? (Y/N)", 0, 14, DST_HUD, 5, 10);
            video_split_slide(-1, 175, 200);
            int k;
            key_flush();
            do { video_wait_vsync(); k = key_pressed_scancode(); if (quit_requested) k = 0x15; } while (k != 0x15 && k != 0x31);
            g_quit_match = k == 0x15;
            video_split_slide(1, 200, 175);
        } else g_quit_match = 1;
    }
    return g_quit_match;
}

static void update_all_players(int include_near, int include_far)
{
    if (include_far)  { player_update(pl_far[0]);  if (g_doubles) player_update(pl_far[1]); }
    if (include_near) { player_update(pl_near[0]); if (g_doubles) player_update(pl_near[1]); }
}

/* 1000:7478 - message on the status bar (GAME, SET, MATCH, CHANGE SIDE...) with an optional sound, while the
 * players keep animating; leaves after ~2 s. */
void show_message(int snd, const char *msg)
{
    font_select(5);
    dst_image(DST_HUD, img_splitmsg, 0, 0);
    text_outlined(msg, 0, 14, DST_HUD, 5, 10);
    font_select(6);
    video_wait_vsync();
    video_split_slide(-1, 175, 200);
    uint32_t t0 = timer_ticks();
    int played = 0;
    for (;;) {
        camera_update();
        update_all_players(1, 1);
        pause_check();
        boss_check();
        if (quit_check(0)) break;
        if (!snd_busy() && !played) { if (snd) snd_play(snd); played = 1; }
        uint32_t dt = timer_ticks() - t0;
        if (dt > 0x24 * 65536u + 0x69df) break;
        if (quit_requested) break;
    }
    video_split_slide(1, 200, 175);
    for (int i = 0; i < 29; i++) video_wait_vsync();
}

/* ---------------------------------------------------------------- serve */

static int near_serving_idx(void)
{
    return (g_doubles && pl_near[1]->anim == 0x15) ? 1 : 0;
}

void serve_near(int practice)
{
    ball.dir = 0;
    int served = 0;
    TPlayer *srv = pl_near[near_serving_idx()];
    do {
        player_reset(pl_near[0]);
        if (g_doubles) player_reset(pl_near[1]);
        int x = (srv->pos == 1) * 0x43 + 0xbc;
        ball_throw(&ball, 2, 0x11, 0x12, 0xe6, x, 0xe6, x);
        do {
            timer_start(g_frame_timer);
            if (!ball_step(&ball, 0)) ball_throw(&ball, 2, 0, 0x12, 0xe6, x, 0xe6, x);
            camera_update();
            update_all_players(1, 0);
            if (!practice) update_all_players(0, 1);
            pause_check(); boss_check();
            frame_wait();
            quit_check(practice);
        } while (!g_quit_match && srv->anim != 0x16);
        if (g_quit_match) return;
        ball_throw(&ball, 2, 0xf, 0x41, 0xe6, x, 0xe6, x);
        while (ball_step(&ball, 1)) {
            timer_start(g_frame_timer);
            camera_update();
            if (ball.bounces != 0) break;
            update_all_players(1, 0);
            if (!practice) update_all_players(0, 1);
            pause_check(); boss_check();
            if (srv->anim != 0x16 && srv->frame > 4) {
                ball_shot(&ball, srv->anim);
                served = 1;
                break;
            }
            if (quit_check(practice)) { served = 1; break; }
            frame_wait();
        }
    } while (!served);
}

void serve_far(int practice)
{
    ball.dir = 1;
    int served = 0;
    TPlayer *srv = pl_far[(g_doubles && pl_far[1]->anim == 0x15) ? 1 : 0];
    do {
        player_reset(pl_far[0]);
        if (g_doubles) player_reset(pl_far[1]);
        int x = (srv->pos == 3) * 0x36 + 0xaa;
        ball_throw(&ball, 2, 0xe, 0xf, 0x5d, x, 0x5d, x);
        do {
            timer_start(g_frame_timer);
            if (!ball_step(&ball, 0)) ball_throw(&ball, 2, 0, 0xf, 0x5d, x, 0x5d, x);
            camera_update();
            update_all_players(0, 1);
            if (!practice) update_all_players(1, 0);
            pause_check(); boss_check();
            frame_wait();
            quit_check(practice);
        } while (!g_quit_match && srv->anim != 0x16);
        if (g_quit_match) return;
        ball_throw(&ball, 2, 0xf, 0x41, 0x5d, x, 0x5d, x);
        while (ball_step(&ball, 1)) {
            timer_start(g_frame_timer);
            camera_update();
            if (ball.bounces != 0) break;
            update_all_players(0, 1);
            if (!practice) update_all_players(1, 0);
            pause_check(); boss_check();
            if (srv->anim != 0x16 && srv->frame > 4) {
                ball_shot(&ball, srv->anim);
                served = 1;
                break;
            }
            if (quit_check(practice)) { served = 1; break; }
            frame_wait();
        }
    } while (!served);
}

/* ---------------------------------------------------------------- point */

int play_point(void)
{
    int result = 0;
    int serve_try = 1;
    int fault = 0, out = 0;

    video_wait_vsync();
    g_out_flag = 0; g_replay_off = 0; g_quit_match = 0; g_rec_idx = 0; g_rec_count = 0x2aa3;

    for (;;) {
        spr_hide(H_OUT);
        g_rally = 1;
        g_first_serve = serve_try == 1;
        int far_serves = pl_far[0]->pos == 3 || pl_far[0]->pos == 4 ||
                         (g_doubles && (pl_far[1]->pos == 3 || pl_far[1]->pos == 4));
        if (far_serves) serve_far(0); else serve_near(0);
        if (g_quit_match) break;

        fault = 0; out = 0; g_out_flag = 0;
        for (;;) {
            timer_start(g_frame_timer);
            if (!ball_step(&ball, 1)) break;
            camera_update();
            ball_net(&ball);
            if (!out && ball.bounces == 1) out = ball_out(&ball); else if (out) out = 1;
            g_out_flag = g_out_flag || ball.alive;
            if (out) spr_show(H_OUT);
            fault = fault || (g_rally && (ball.alive || out));
            if (!fault && !g_rally && !out && ball.bounces < 2) {
                if (!ball.dir) {
                    if (player_hits_ball(pl_near[0])) ball_shot(&ball, pl_near[0]->anim);
                    else if (g_doubles && player_hits_ball(pl_near[1])) ball_shot(&ball, pl_near[1]->anim);
                } else {
                    if (player_hits_ball(pl_far[0])) ball_shot(&ball, pl_far[0]->anim);
                    else if (g_doubles && player_hits_ball(pl_far[1])) ball_shot(&ball, pl_far[1]->anim);
                }
            }
            player_update(pl_far[0]);
            player_update(pl_near[0]);
            if (g_doubles) { player_update(pl_far[1]); player_update(pl_near[1]); }
            g_rally = g_rally && ball.bounces == 0;
            pause_check(); boss_check();
            frame_wait();
            if (quit_check(0)) break;
        }
        if (g_quit_match || !fault) break;
        if (serve_try == 1) players_reset_all();
        if (serve_try == 2) break;
        serve_try++;
    }

    /* end of the point: who won? */
    spr_hide(H_OUT);
    g_rec_count = g_rec_idx - 1;
    if (!g_quit_match) {
        int clean = !fault && !out && !g_out_flag;
        int winner;
        if (!ball.dir) winner = clean ? pl_far[0]->id : pl_near[0]->id;
        else           winner = clean ? pl_near[0]->id : pl_far[0]->id;
        result = score_award(&score, winner);
    } else {
        int winner = !pl_far[0]->human ? pl_far[0]->id : pl_near[0]->id;
        result = score_award(&score, winner);
    }
    g_replay_off = 1;
    if (!g_quit_match) snd_play((!fault && !out && !g_out_flag) ? 7 : 8);
    return result;
}

/* ---------------------------------------------------------------- game / match */

static void ball_hide_all(void) { spr_hide(H_BALL); spr_hide(H_SHADOW); }
static void ball_show_all(void) { spr_show(H_BALL); spr_show(H_SHADOW); }

int play_game(void)
{
    int r;
    for (;;) {
        ball_show_all();
        r = play_point();
        ball_hide_all();
        if (!g_quit_match) {
            if (r != 0) {
                char msg[64];
                int snd;
                if (r == 2) { snprintf(msg, sizeof msg, "%s%s", ds_cstr(0xe9e), score_last_name(&score)); snd = 28; }
                else if (r == 3) { snprintf(msg, sizeof msg, "%s%s", ds_cstr(0xeaa), score_last_name(&score)); snd = 29; }
                else { snprintf(msg, sizeof msg, "%s%s", ds_cstr(0xebd), score_last_name(&score)); snd = 27; }
                show_message(snd, msg);
            }
            if (g_quit_match) return r;
            hud_show_scoreboard(&score);
            video_split_slide(-1, 175, 200);
            uint32_t t0 = timer_ticks();
            int announced = 0;
            for (;;) {
                camera_update();
                update_all_players(1, 1);
                pause_check(); boss_check();
                if (keys[0x3d] && !g_quit_match) { video_split_slide(1, 200, 175); replay_play(); video_split_slide(-1, 175, 200); }
                if (quit_check(0)) break;
                if (!snd_busy() && !announced) { int id = score_announce_id(&score); if (id) snd_play(id); announced = 1; }
                uint32_t dt = timer_ticks() - t0;
                if (dt > 0x36 * 65536u + 0x9ecf) break;
                if (quit_requested) break;
            }
            video_split_slide(1, 200, 175);
            for (int i = 0; i < 29; i++) video_wait_vsync();
            if (g_quit_match) return r;
            if ((r != 0 && score_change_ends(&score)) || (score.tiebreak && score_change_ends(&score)))
                show_message(0, ds_cstr(0xed2));            /* CHANGE SIDE */
        }
        if (g_quit_match || r != 0) return r;
        if (!score.tiebreak) {
            players_apply_pos(0, 0);
        } else if (!score_change_ends(&score)) {
            if (!score_odd(&score)) players_apply_pos(0, 0);
            else {
                players_apply_pos(0, 1);
                players_apply_pos(0, 0);
                show_message(0, ds_cstr(0xede));            /* CHANGE SERVE */
                score.server_flag ^= 1;
            }
        } else {
            if (!score_odd(&score)) { players_apply_pos(1, 1); players_apply_pos(0, 1); }
            else {
                players_apply_pos(1, 1);
                show_message(0, ds_cstr(0xede));
                score.server_flag ^= 1;
            }
            players_swap_sides();
        }
        players_reset_all();
        if (g_quit_match) return r;
    }
}

int play_match(void)
{
    players_set_team_ids();
    do {
        int r = play_game();
        if (g_quit_match || r == 3) break;
        if (score.tiebreak) show_message(30, ds_cstr(0xeec));      /* TIEBREAK */
        int ends = score_change_ends(&score);
        players_apply_pos(ends, 1);
        if (ends) players_swap_sides();
        players_reset_all();
    } while (!g_quit_match);
    if (!g_quit_match) return score.last;
    return pl_far[0]->human ? pl_near[0]->id : pl_far[0]->id;
}
