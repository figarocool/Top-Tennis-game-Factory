#include "dsimg.h"
#include "hof.h"
#include "score.h"
#include "video.h"
#include "text.h"
#include "options.h"
#include "ui.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    char    name1[21], name2[21];
    uint8_t games[10];            /* p1 sets 1..5, p2 sets 1..5 */
    char    desc[31];
} HofRec;                         /* 83 bytes */

static HofRec rec[8];             /* 1..7 */

/* in-memory records keep C strings; on disk they are Pascal strings (length byte, then characters) */
static void unpack(char *s, int max)
{
    int l = (uint8_t)s[0];
    if (l > max) l = max;
    memmove(s, s + 1, l);
    s[l] = 0;
}

static void pack(char *s, int max)
{
    int l = (int)strlen(s);
    if (l > max) l = max;
    memmove(s + 1, s, l);
    s[0] = (char)l;
}

static void hof_path(char *p, size_t n) { snprintf(p, n, "%s/TENNIS.HAL", data_dir); }

void hof_load(void)
{
    memset(rec, 0, sizeof rec);
    char p[600]; hof_path(p, sizeof p);
    FILE *f = fopen(p, "rb");
    if (!f) return;
    uint8_t raw[7 * 83];
    if (fread(raw, 1, sizeof raw, f) == sizeof raw)
        for (int i = 1; i <= 7; i++) memcpy(&rec[i], raw + (i - 1) * 83, 83);
    for (int i = 1; i <= 7; i++) {            /* the file holds Pascal strings: length byte first */
        unpack(rec[i].name1, 20); unpack(rec[i].name2, 20); unpack(rec[i].desc, 30);
    }
    fclose(f);
}

static void hof_save(void)
{
    char p[600]; hof_path(p, sizeof p);
    FILE *f = fopen(p, "wb");
    if (!f) return;
    for (int i = 1; i <= 7; i++) {
        HofRec r = rec[i];
        memset(r.name1 + 1 + strlen(r.name1), 0, 0);
        pack(r.name1, 20); pack(r.name2, 20); pack(r.desc, 30);
        fwrite(&r, 1, 83, f);
    }
    fclose(f);
}

static void show_bg(Palette *pal)
{
    memset(vpage, 0, sizeof vpage);
    Image *im = img_load_pbm("DATA\\BACKGND\\HFAME.PBM");
    if (im) { blit(vpage, VW, VH, im, 0, 0); img_free(im); }
    pal_load("DATA\\PAL\\HFAME.PAL", pal);
}

/* 1000:13b2 */
static void draw_rows(void)
{
    font_select(7);
    int y1 = 0x22, y2 = 0x29;
    for (int i = 1; i <= 7; i++, y1 += 0x19, y2 += 0x19) {
        fill_rect(vpage, VW, VH, 0x21, y1 - 6, 0xde - 0x21, (y2 + 8) - (y1 - 6), 8);
        fill_rect(vpage, VW, VH, 0x1f, y1 - 8, 0xdc - 0x1f, (y1 - 1) - (y1 - 8), 1);
        fill_rect(vpage, VW, VH, 0x1f, y1 - 1, 0xdc - 0x1f, (y2 + 6) - (y1 - 1), 7);
        const HofRec *r = &rec[i];
        if (!r->games[0] && !r->games[5]) continue;
        text_at(r->desc, 3, DST_PAGE, y1 - 7, 0x21);
        text_at(r->name1, 1, DST_PAGE, y1, 0x21);
        text_at(r->name2, 1, DST_PAGE, y2, 0x21);
        static const int xs[5] = { 0xaa, 0xb4, 0xbe, 200, 0xd2 };
        for (int s = 0; s < 5; s++) {
            if (s >= 2 && !r->games[s] && !r->games[s + 5]) continue;
            char b[8];
            snprintf(b, sizeof b, "%2d", r->games[s]);     text_at(b, 1, DST_PAGE, y1, xs[s]);
            snprintf(b, sizeof b, "%2d", r->games[s + 5]); text_at(b, 1, DST_PAGE, y2, xs[s]);
        }
    }
}

int act_hall_of_fame(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    Palette pal;
    video_fade_out(2);
    show_bg(&pal);
    draw_rows();
    video_fade_in(&pal, 2);
    key_flush();
    for (;;) {
        video_wait_vsync();
        if (key_pressed_scancode() || quit_requested) break;
    }
    font_select(5);
    video_fade_out(2);
    ui_dirty = 1;
    return MR_STAY;
}

/* 1000:17b8 - choose a row with Up/Down, Enter takes it, ESC refuses (returns 0) */
static int pick_slot(void)
{
    static uint8_t base[VW * VH];
    memcpy(base, vpage, sizeof base);
    int slot = 1;
    key_flush();
    for (;;) {
        memcpy(vpage, base, sizeof base);
        int y = slot * 0x19, x = 0x14, w = 0x35, h = 0x19;
        fill_rect(vpage, VW, VH, x, y, w, 1, 14);
        fill_rect(vpage, VW, VH, x, y + h - 1, w, 1, 14);
        fill_rect(vpage, VW, VH, x, y, 1, h, 14);
        fill_rect(vpage, VW, VH, x + w - 1, y, 1, h, 14);
        video_wait_vsync();
        int k = key_pressed_scancode();
        if (quit_requested) return 0;
        if (k == 0xc8 && slot > 1) slot--;
        else if (k == 0xd0 && slot < 7) slot++;
        else if (k == 0x1c) return slot;
        else if (k == 0x01) return 0;
    }
}

/* 1000:1898 (+1345, 12e4) */
void hof_offer_save(const char *desc, const char *extra)
{
    Palette pal;
    video_fade_out(2);
    video_set_scroll(0, 0);
    show_bg(&pal);
    font_select(5);
    text_outlined(ds_cstr(0xe62), 0, 10, DST_PAGE, 1, 5);
    draw_rows();
    video_fade_in(&pal, 2);
    int slot = pick_slot();
    if (slot) {
        HofRec *r = &rec[slot];
        memset(r, 0, sizeof *r);
        snprintf(r->name1, sizeof r->name1, "%.20s", score.name[0]);
        snprintf(r->name2, sizeof r->name2, "%.20s", score.name[1]);
        for (int s = 0; s < 5; s++) { r->games[s] = score.games[0][s]; r->games[s + 5] = score.games[1][s]; }
        snprintf(r->desc, sizeof r->desc, "%s %s", desc, extra ? extra : "");
        hof_save();
        show_bg(&pal);
        font_select(7);
        draw_rows();
        video_set_palette(&pal);
        video_present();
        key_flush();
        for (;;) { video_wait_vsync(); if (key_pressed_scancode() || quit_requested) break; }
    }
    font_select(5);
    video_fade_out(2);
}
