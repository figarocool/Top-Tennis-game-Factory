/* Menu actions that are simple screens: quit confirmation, credits, options load/save, keyboard setup.
 * The signature of every action is (menu, item) -> MR_* code. Each one ends with the screen faded out so
 * the menu can fade back in (the original does the same with FUN_1010_38d3 / 39cc). */
#include "menu.h"
#include "ui.h"
#include "dialog.h"
#include "text.h"
#include "video.h"
#include "options.h"
#include "screens.h"
#include "dsimg.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

static void wait_any_key(void)
{
    for (;;) {
        plat_poll();
        if (quit_requested) return;
        for (int i = 1; i < 256; i++) if (keys[i]) return;
        video_wait_vsync();
    }
}

static void wait_all_keys_up(void)
{
    for (;;) {
        plat_poll();
        int any = 0;
        for (int i = 1; i < 256; i++) if (keys[i]) any = 1;
        if (!any || quit_requested) return;
        video_wait_vsync();
    }
}

/* 1000:cce6 + dfa4: "QUIT GAME / Sure?" with YES and NO */
int act_exit(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    Dialog d;
    dialog_init(&d, ds_cstr(0x2d65), 5, 0x1b, 0x60, 0x32);
    dialog_add_button(&d, ds_cstr(0x3067), 1, 0x8a, 0x46);
    dialog_add_button(&d, ds_cstr(0x306e), 0, 0x8a, 0xbe);
    dialog_add_label(&d, ds_cstr(0x3075), 0x78, 100);
    font_select(8);
    int r = dialog_run(&d);
    font_select(5);
    return r == 1 ? MR_CLOSE : MR_STAY;
}

/* 1000:cd39 */
int act_credits(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    video_fade_out(2);
    screen_show_image("DATA\\BACKGND\\CREDITS.PBM");
    Palette p;
    pal_load("DATA\\PAL\\CREDITS.PAL", &p);
    {   /* the port's credit, under "Created by John Dolph", in the blue of that line */
        int best = 1, bd = 1 << 30;
        for (int i = 1; i < 256; i++) {
            int dr = p.c[i][0] - 10, dg = p.c[i][1] - 28, db = p.c[i][2] - 62;        /* 6-bit DAC values of a bright blue */
            int d = dr * dr + dg * dg + db * db;
            if (d < bd) { bd = d; best = i; }
        }
        const char *line = "porting by Stefano Basile";
        font_select(5);
        text_at(line, best, DST_PAGE, 56, 190 - text_pix_width(line) / 2 + 10);
    }
    video_fade_in(&p, 2);
    wait_all_keys_up();
    wait_any_key();
    video_fade_out(2);
    ui_dirty = 1;
    return MR_STAY;
}

int act_save_options(Menu *m, MenuItem *it) { (void)m; (void)it; opt_save(); return MR_STAY; }
int act_load_options(Menu *m, MenuItem *it) { (void)m; (void)it; opt_load(); return MR_STAY; }

/* 1000:a357 - redefine the keyboard: TECLAT.PBM with the current keys; each one is captured in turn */
static int redefine_keys(int set)
{
    uint8_t *k = opt.keys[set];
    Palette p;
    video_fade_out(2);
    screen_show_image("DATA\\BACKGND\\TECLAT.PBM");
    pal_load("DATA\\PAL\\TECLAT.PAL", &p);
    font_select(5);
    const struct { const char *t; int y, x, fill; } labels[] = {
        { ds_cstr(0x1a6c), 0x11, 0x14, 14 }, { ds_cstr(0x1a72), 0x25, 0x14, 14 }, { ds_cstr(0x1a78), 0x11, 0x8e, 14 },
        { ds_cstr(0x1a7f), 0x25, 0x8e, 14 }, { "UP", 0x46, 0x32, 14 }, { ds_cstr(0x1a90), 0x5a, 0x32, 14 },
        { ds_cstr(0x1a95), 0x6e, 0x32, 14 }, { ds_cstr(0x1a9a), 0x82, 0x32, 14 }, { ds_cstr(0x1aa0), 0x96, 0x32, 14 },
        { ds_cstr(0x1aa5), 0xb4, 0xf0, 10 }, { "F5", 0x11, 0x50, 3 }, { "ESC", 0x25, 0x50, 3 },
        { "F3", 0x11, 0x104, 3 }, { "F10", 0x25, 0x104, 3 },
    };
    for (unsigned i = 0; i < sizeof labels / sizeof labels[0]; i++)
        text_outlined(labels[i].t, 0, labels[i].fill, DST_PAGE, labels[i].y, labels[i].x);
    static const int ys[5] = { 0x46, 0x5a, 0x6e, 0x82, 0x96 };
    for (int i = 0; i < 5; i++) text_outlined(ds_key_name(k[i]), 0, 3, DST_PAGE, ys[i], 0x78);
    static uint8_t base[VW * VH];
    memcpy(base, vpage, sizeof base);
    video_fade_in(&p, 2);
    wait_all_keys_up();

    int ok = 1;
    for (int i = 0; i < 5 && ok; i++) {
        int blink = 0, cnt = 0, got = 0;
        key_flush();
        while (!got && !quit_requested) {
            if (++cnt > 20) { cnt = 0; blink ^= 1; }
            memcpy(vpage, base, sizeof base);
            if (blink) {                                  /* yellow frame around the slot */
                int x = 0x78, y = ys[i];
                ui_bevel(14, 14, 1, y + 0xd, x + 0x5b, y - 1, x - 2);
            }
            video_wait_vsync();
            int sc = key_pressed_scancode();
            if (!sc) continue;
            if (sc == 0x01) { ok = 0; break; }
            if (sc == 0x2a || sc == 0x36 || sc == 0x1d || sc == 0x38 || sc == 0x3a || sc == 0x9d || sc == 0xb8) continue;
            k[i] = (uint8_t)sc;
            memcpy(vpage, base, sizeof base);
            ui_fill(0, 0, 0, 0, 0);
            text_outlined(ds_key_name(sc), 0, 3, DST_PAGE, ys[i], 0x78);
            memcpy(base, vpage, sizeof base);
            got = 1;
            wait_all_keys_up();
        }
    }
    if (ok) for (int i = 0; i < 70; i++) video_wait_vsync();
    video_fade_out(2);
    ui_dirty = 1;
    return ok;
}

int act_redefine_kb1(Menu *m, MenuItem *it) { (void)m; (void)it; redefine_keys(0); return MR_STAY; }
int act_redefine_kb2(Menu *m, MenuItem *it) { (void)m; (void)it; redefine_keys(1); return MR_STAY; }
