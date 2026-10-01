/* Replay of the last point (F3) and REPLAY.nnn files. The buffers hold {x, y, sprite id} per frame. */
#include "replay.h"
#include "match.h"
#include "game.h"
#include "gamecfg.h"
#include "player.h"
#include "sprites.h"
#include "video.h"
#include "text.h"
#include "dialog.h"
#include "options.h"
#include "platform.h"
#include "savefile.h"
#include "ui.h"
#include "sound.h"
#include <string.h>

enum { H_SHADOW = 5, H_BALL = 6 };
#define KEY_LEFT 0xcb
#define KEY_RIGHT 0xcd

static void show_frame(int idx)                     /* 1000:7d2f */
{
    timer_start(g_frame_timer);
    ball_replay(&ball, idx);
    player_replay(pl_far[0], idx);
    player_replay(pl_near[0], idx);
    if (g_doubles) { player_replay(pl_far[1], idx); player_replay(pl_near[1], idx); }
    camera_update();
    frame_wait();
}

static void save_replay_dialog(void)                /* 1000:7e72 */
{
    while (keys[0x1f]) { plat_poll(); video_wait_vsync(); }
    int idx = dlg_slots("        SAVE REPLAY", "   DELETE REPLAY FILE", "REPLAY", 1);
    if (idx >= 0) {
        char desc[SAVE_DESC + 1];
        memset(desc, 0, sizeof desc);
        if (idx < slot_count("REPLAY")) slot_read_desc("REPLAY", idx, desc);
        if (dlg_description(desc, 24)) {
            FILE *f = slot_open("REPLAY", idx, "wb");
            if (f) {
                slot_write_desc(f, desc);
                fputc(g_doubles, f);
                fwrite(&g_rec_count, 2, 1, f);
                fputc(g_court_type, f);
                size_t n = ((size_t)g_rec_count + 1) * 6;
                fwrite(ball.rec_ball, 1, n, f);
                fwrite(ball.rec_shadow, 1, n, f);
                TPlayer *ps[4] = { pl_far[0], pl_near[0], pl_far[1], pl_near[1] };
                for (int i = 0; i < 4; i++) fwrite(ps[i]->rec, 1, n, f);
                fclose(f);
            }
        }
    }
    font_select(5);
    while (key_pressed_scancode()) {}
}

void replay_play(void)
{
    video_split_slide(-1, 190, 200);
    font_select(8);
    g_replay_off = 1;
    spr_show(H_BALL); spr_show(H_SHADOW);
    int idx = 1;
    while (!keys[1] && !quit_requested) {
        show_frame(idx);
        plat_poll();
        if (keys[KEY_RIGHT] && idx < g_rec_count) idx++;
        else if (keys[KEY_LEFT] && idx > 1) idx--;
        if (keys[0x1f]) save_replay_dialog();
        pause_check();
        boss_check();
    }
    spr_hide(H_BALL); spr_hide(H_SHADOW);
    while (keys[1] && !quit_requested) { plat_poll(); video_wait_vsync(); }
    video_split_slide(1, 200, 190);
    font_select(5);
}

/* 1000:c889 */
int act_load_replay(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    int idx = dlg_slots("        LOAD REPLAY", "   DELETE REPLAY FILE", "REPLAY", 0);
    if (idx < 0) { ui_dirty = 1; return MR_STAY; }
    FILE *f = slot_open("REPLAY", idx, "rb");
    if (!f) return MR_STAY;
    char d[SAVE_DESC];
    int doubles = 0, court = 1;
    uint16_t count = 0;
    int ok = fread(d, 1, SAVE_DESC, f) == SAVE_DESC;
    doubles = fgetc(f);
    ok = ok && fread(&count, 2, 1, f) == 1;
    court = fgetc(f);
    if (!ok || count > 0x2aa3) { fclose(f); return MR_STAY; }

    video_fade_out(2);
    game_make_sprites();
    g_doubles = doubles;
    g_rec_count = count;
    size_t n = ((size_t)count + 1) * 6;
    int good = fread(ball.rec_ball, 1, n, f) == n && fread(ball.rec_shadow, 1, n, f) == n;
    TPlayer *ps[4] = { pl_far[0], pl_near[0], pl_far[1], pl_near[1] };
    for (int i = 0; i < 4 && good; i++) good = fread(ps[i]->rec, 1, n, f) == n;
    fclose(f);
    if (good) {
        court_select(court);
        for (int i = 0; i < 4; i++) player_show(&players[i]);
        if (!g_doubles) { player_hide(pl_far[1]); player_hide(pl_near[1]); }
        match_set_speed(63 + 7 * g_doubles);
        Palette pal = vpal;
        video_black();
        video_set_split(200);
        spr_render(0, 0);
        video_fade_in(&pal, 2);
        replay_play();
        video_set_scroll(0, 0);
        video_fade_out(2);
    }
    ui_dirty = 1;
    return MR_STAY;
}
