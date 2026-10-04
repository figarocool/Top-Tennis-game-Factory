#include "dsimg.h"
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
#include <stdlib.h>

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
    int idx = dlg_slots(ds_cstr(0x2fd2), ds_cstr(0x2d0f), ds_cstr(0x1a78), 1);
    if (idx >= 0) {
        char desc[SAVE_DESC + 1];
        memset(desc, 0, sizeof desc);
        if (idx < slot_count(ds_cstr(0x1a78))) slot_read_desc(ds_cstr(0x1a78), idx, desc);
        if (dlg_description(desc, 24)) {
            FILE *f = slot_open(ds_cstr(0x1a78), idx, "wb");
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
    if (!g_rec_count || g_rec_count >= 0x2aa4) return;
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

typedef struct {
    uint16_t count;
    uint8_t doubles, court;
    uint8_t *frames;
} ReplayFile;

/* Read everything before changing the live replay or creating its sprites. */
static int read_replay(FILE *f, ReplayFile *out)
{
    uint8_t description[SAVE_DESC], header[4];
    memset(out, 0, sizeof *out);
    if (fread(description, 1, sizeof description, f) != sizeof description ||
        description[0] >= SAVE_DESC || fread(header, 1, sizeof header, f) != sizeof header) return 0;
    unsigned count = header[1] | header[2] << 8;
    if (header[0] > 1 || header[3] < 1 || header[3] > 4 || count > 0x2aa3) return 0;
    size_t bytes = ((size_t)count + 1) * 6 * 6;
    uint8_t *frames = malloc(bytes);
    if (!frames) return 0;
    if (fread(frames, 1, bytes, f) != bytes) { free(frames); return 0; }
    out->count = count; out->doubles = header[0]; out->court = header[3]; out->frames = frames;
    return 1;
}

/* 1000:c889 */
int act_load_replay(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    int idx = dlg_slots(ds_cstr(0x2fa8), ds_cstr(0x2d0f), ds_cstr(0x1a78), 0);
    if (idx < 0) { ui_dirty = 1; return MR_STAY; }
    FILE *f = slot_open(ds_cstr(0x1a78), idx, "rb");
    if (!f) return MR_STAY;
    ReplayFile loaded;
    int ok = read_replay(f, &loaded);
    fclose(f);
    if (!ok) { ui_dirty = 1; return MR_STAY; }

    video_fade_out(2);
    game_make_sprites();
    TPlayer *ps[4] = { pl_far[0], pl_near[0], pl_far[1], pl_near[1] };
    int good = ball.rec_ball && ball.rec_shadow;
    for (int i = 0; i < 4; i++) if (!ps[i]->rec) good = 0;
    if (good) {
        g_doubles = loaded.doubles;
        g_rec_count = loaded.count;
        size_t n = ((size_t)loaded.count + 1) * 6;
        memcpy(ball.rec_ball, loaded.frames, n);
        memcpy(ball.rec_shadow, loaded.frames + n, n);
        for (int i = 0; i < 4; i++) memcpy(ps[i]->rec, loaded.frames + (i + 2) * n, n);
        court_select(loaded.court);
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
    free(loaded.frames);
    ui_dirty = 1;
    return MR_STAY;
}
