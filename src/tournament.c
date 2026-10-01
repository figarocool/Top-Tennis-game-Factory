/* Tournaments and the full season (1000:8318..9dec, ce2c). A tournament is a 64-player knock-out draw; the
 * human players always start at the bottom of the ranking. Matches between two computer players are
 * simulated (8689), the others are played. */
#include "tournament.h"
#include "matchflow.h"
#include "game.h"
#include "player.h"
#include "sprites.h"
#include "video.h"
#include "text.h"
#include "dialog.h"
#include "dsimg.h"
#include "options.h"
#include "ui.h"
#include "screens.h"
#include "platform.h"
#include "match.h"
#include "score.h"
#include "hof.h"
#include "sound.h"
#include "savefile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

DbPlayer db[NPLAYERS + 1];

typedef struct {
    const char *name, *pal, *bg;
    int court;           /* 1 clay, 2 hard, 3 grass, 4 indoor */
    int final5;          /* the final is played over 5 sets */
    int pts[7];          /* ranking points for winning a match of round 1..6 */
} Tournament;

static const Tournament tour[13] = {
    { 0 },
    { "SYDNEY",        "DATA\\PAL\\SYDNEY.PAL",   "DATA\\BACKGND\\SYDNEY.PBM",   2, 0, {0, 10, 10, 10, 10, 20, 20} },
    { "MELBOURNE",     "DATA\\PAL\\MELBOURN.PAL", "DATA\\BACKGND\\MELBOURN.PBM", 2, 1, {0, 10, 10, 10, 10, 20, 20} },
    { "SAN FRANCISCO", "DATA\\PAL\\SANFRAN.PAL",  "DATA\\BACKGND\\SANFRAN.PBM",  2, 0, {0, 10, 10, 10, 10, 20, 20} },
    { "TOKYO",         "DATA\\PAL\\TOKIO.PAL",    "DATA\\BACKGND\\TOKIO.PBM",    4, 0, {0, 10, 10, 10, 10, 20, 20} },
    { "BARCELONA",     "DATA\\PAL\\BARCELON.PAL", "DATA\\BACKGND\\BARCELON.PBM", 1, 0, {0, 10, 10, 10, 10, 20, 20} },
    { "PARIS",         "DATA\\PAL\\PARIS.PAL",    "DATA\\BACKGND\\PARIS.PBM",    1, 1, {0, 10, 10, 10, 10, 20, 20} },
    { "LONDON",        "DATA\\PAL\\LONDRES.PAL",  "DATA\\BACKGND\\LONDRES.PBM",  3, 1, {0, 10, 10, 10, 10, 20, 20} },
    { "MONTREAL",      "DATA\\PAL\\MONTREAL.PAL", "DATA\\BACKGND\\MONTREAL.PBM", 2, 0, {0, 10, 10, 10, 10, 20, 20} },
    { "NEW YORK",      "DATA\\PAL\\NEWYORK.PAL",  "DATA\\BACKGND\\NEWYORK.PBM",  2, 1, {0, 10, 10, 10, 10, 20, 20} },
    { "STOCKHOLM",     "DATA\\PAL\\ESTOCOLM.PAL", "DATA\\BACKGND\\ESTOCOLM.PBM", 1, 0, {0, 10, 10, 10, 10, 20, 20} },
    { "MOSCOW",        "DATA\\PAL\\MOSCU.PAL",    "DATA\\BACKGND\\MOSCU.PBM",    1, 0, {0, 10, 10, 10, 10, 20, 20} },
    { "FRANKFURT",     "DATA\\PAL\\FRANKFUR.PAL", "DATA\\BACKGND\\FRANKFUR.PBM", 4, 0, {0, 10, 10, 10, 10, 20, 20} },
};

static uint8_t bracket[NPLAYERS + 2][8];
static int saved_id, saved_round;      /* [position][round] -> db index (DS:A163 + pos*6 + round) */

/* ------------------------------------------------------------ player database */

void db_init(void)
{
    memset(db, 0, sizeof db);
    for (int i = 1; i <= NPLAYERS; i++) {
        snprintf(db[i].name, sizeof db[i].name, "%s", ds_cstr(0x1074 + (i - 1) * 19));
        db[i].points = i <= 60 ? 0xf0 - 4 * (i - 1) : 0;
        db[i].ctrl = 5;
        db[i].level = i < 0xb ? 4 : i < 0x24 ? 3 : i < 0x37 ? 2 : 1;
    }
}

static void db_qsort(int hi, int lo)
{
    int i = lo, j = hi;
    DbPlayer pivot = db[(lo + hi) / 2];
    do {
        while (pivot.points < db[i].points) i++;
        while (db[j].points < pivot.points) j--;
        if (i <= j) { DbPlayer t = db[i]; db[i] = db[j]; db[j] = t; i++; j--; }
    } while (i <= j);
    if (lo < j) db_qsort(j, lo);
    if (i < hi) db_qsort(hi, i);
}

void db_sort(void) { db_qsort(NPLAYERS, 1); }

static int db_len(const char *n) { int l = (int)strlen(n); while (l && n[l - 1] == ' ') l--; return l; }

/* 1000:8689 - who wins a computer-vs-computer match */
static int simulate(int a, int b)
{
    int p = 5;
    if (b < a) p = 6; else if (a < b) p = 4;
    if (db[a].level < db[b].level) p += 2; else if (db[b].level < db[a].level) p -= 2;
    return rnd(10) < p ? b : a;
}

/* ------------------------------------------------------------ screens */

static void wait_key_press(void)
{
    for (;;) {
        plat_poll();
        if (quit_requested) return;
        for (int i = 1; i < 256; i++) if (keys[i]) return;
        video_wait_vsync();
    }
}

static void wait_keys_released(void)
{
    for (;;) {
        plat_poll();
        int any = 0;
        for (int i = 1; i < 256; i++) if (keys[i]) any = 1;
        if (!any || quit_requested) return;
        video_wait_vsync();
    }
}

/* the dimmed variant of a city picture / palette: the character before the '.' becomes '2' (SYDNEY -> SYDNE2) */
static void variant2(char *dst, const char *src)
{
    snprintf(dst, 96, "%s", src);
    char *dot = strrchr(dst, '.');
    if (dot && dot > dst) dot[-1] = '2';
}

static void show_page(const Palette *p)
{
    video_set_scroll(0, 0);
    video_set_split(200);
    video_set_palette(p);
    video_present();
}

static void draw_city2(int id, Palette *pal)
{
    char bg[96], pl[96];
    variant2(bg, tour[id].bg);
    variant2(pl, tour[id].pal);
    memset(vpage, 0, sizeof vpage);
    Image *im = img_load_pbm(bg);
    if (im) { blit(vpage, VW, VH, im, 0, 0); img_free(im); }
    pal_load(pl, pal);
}

static void overlay(const char *pbm)               /* FUN_1000_82c1: colour 0 is transparent */
{
    Image *im = img_load_pbm(pbm);
    if (!im) return;
    im->mask = malloc((size_t)im->w * im->h);
    for (int i = 0; i < im->w * im->h; i++) im->mask[i] = im->px[i] != 0;
    blit(vpage, VW, VH, im, 0, 0);
    img_free(im);
}

/* 1000:9232 */
static void tournament_intro(int id)
{
    font_select(5);
    memset(vpage, 0, sizeof vpage);
    Image *im = img_load_pbm(tour[id].bg);
    if (im) { blit(vpage, VW, VH, im, 0, 0); img_free(im); }
    Palette pal;
    pal_load(tour[id].pal, &pal);
    text_outlined("TOURNAMENT N.", 0, 12, DST_PAGE, 1, 5);
    char num[8]; snprintf(num, sizeof num, "%2d", id);
    text_outlined(num, 0, 12, DST_PAGE, 1, 0x73);
    text_outlined(tour[id].name, 0, 14, DST_PAGE, 0xae, 10);
    static const char *ct[5] = { "CLAY COURT", "CLAY COURT", "HARD COURT", "GRASS COURT", "INDOOR COURT" };
    text_outlined(ct[tour[id].court], 0, 2, DST_PAGE, 0xbb, 10);
    show_page(&pal);
}

static const char *ctrl_name(int c)
{
    switch (c) { case 1: return "KEYBOARD 1"; case 2: return "KEYBOARD 2"; case 3: return "JOYSTICK 1"; case 4: return "JOYSTICK 2"; default: return "COMPUTER"; }
}

static const char *round_name(int r)
{
    switch (r) { case 6: return "    FINAL"; case 5: return " SEMIFINALS"; case 4: return "QUARTERFINALS"; case 3: return " 3RD. ROUND"; case 2: return " 2ND. ROUND"; default: return " 1ST. ROUND"; }
}

/* 1000:8318 - "COMING NEXT" screen. left = db index of the first player, right = the second */
static void versus_screen(int left, int right, int round, int id)
{
    Palette pal;
    font_select(5);
    draw_city2(id, &pal);
    overlay("DATA\\BACKGND\\VERSUS.PBM");
    text_outlined("COMING NEXT", 0, 12, DST_PAGE, 2, 0x78);
    text_outlined(round_name(round), 0, 12, DST_PAGE, 200 - 14, 0x72);
    char b[16];
    text_outlined(db[left].name, 0, 14, DST_PAGE, 0x1f, 5);
    text_outlined("RANKING:", 0, 2, DST_PAGE, 0x46, 2);
    snprintf(b, sizeof b, "%2d", left); text_outlined(b, 0, 2, DST_PAGE, 0x46, 0x4a);
    text_outlined("CONTROL:", 0, 2, DST_PAGE, 0x5a, 2);
    text_outlined(ctrl_name(db[left].ctrl), 0, 2, DST_PAGE, 0x5a, 0x4a);
    text_outlined(db[right].name, 0, 14, DST_PAGE, 0x1f, 0x13a - db_len(db[right].name) * 8);
    text_outlined("RANKING:", 0, 2, DST_PAGE, 0x46, 0xa5);
    snprintf(b, sizeof b, "%2d", right); text_outlined(b, 0, 2, DST_PAGE, 0x46, 0xec);
    text_outlined("CONTROL:", 0, 2, DST_PAGE, 0x5a, 0xa5);
    text_outlined(ctrl_name(db[right].ctrl), 0, 2, DST_PAGE, 0x5a, 0xec);
    show_page(&pal);
    wait_keys_released();
    wait_key_press();
    video_fade_out(2);
}

/* 1000:0704 - short name for the draw: the surname (text after the first space), at most 10 characters */
static void short_name(const char *n, char *out)
{
    const char *sp = strchr(n, ' ');
    const char *s = n;
    if (sp) {
        const char *t = sp;
        int only_spaces = 1;
        for (; *t; t++) if (*t != ' ') { only_spaces = 0; break; }
        if (!only_spaces) s = sp + 1;
    }
    snprintf(out, 11, "%s", s);
}

/* 1000:93d6 - the draw sheet */
static void bracket_screen(int winner, int id)
{
    Palette pal;
    draw_city2(id, &pal);
    overlay("DATA\\BACKGND\\QUADRE.PBM");
    font_select(7);
    for (int r = 2; r <= 6; r++) {
        int x = (r - 2) * 0x40 + 2 - (r == 6);
        int y = (6 << (r - 2)) - 1;
        int step = 0xc << (r - 2);
        for (int p = 1; p <= (0x40 >> (r - 1)); p += 2) {
            for (int k = 0; k < 2; k++) {
                int pl = bracket[p + k][r];
                if (!pl) continue;
                char b[16];
                short_name(db[pl].name, b);
                text_at(b, db[pl].ctrl == 5 ? 14 : 10, DST_PAGE, y + 6 * k, x);
            }
            y += step;
        }
    }
    if (winner) {
        text_outlined("WINNER", 0, 10, DST_PAGE, 0xa5, 0x118);
        int l = db_len(db[winner].name);
        text_outlined(db[winner].name, 0, 10, DST_PAGE, 0xaf, (0x14 - l) * 6 + 0xc3);
    }
    text_outlined("CONTINUE  SAVE  QUIT", 0, 9, DST_PAGE, 0xc0, 0xbe);
    text_at("C         S     Q   ", 11, DST_PAGE, 0xc0, 0xbe);
    font_select(5);
    int l = (int)strlen(tour[id].name);
    text_outlined(tour[id].name, 0, 10, DST_PAGE, 5, 0x136 - l * 9);
    font_select(7);
    show_page(&pal);
}

/* 1000:8df4 - the ranking table (64 players, two columns) */
static void ranking_screen(void)
{
    Palette pal;
    memset(vpage, 0, sizeof vpage);
    Image *im = img_load_pbm("DATA\\BACKGND\\RANKING.PBM");
    if (im) { blit(vpage, VW, VH, im, 0, 0); img_free(im); }
    pal_load("DATA\\PAL\\RANKING.PAL", &pal);
    show_page(&pal);
    for (int i = 0; i < 91; i++) video_wait_vsync();
    font_select(7);
    for (int i = 1; i <= NPLAYERS; i++) {
        int x = ((i - 1) >> 5) * 0xa3 + 3, y = ((i - 1) % 0x20) * 6 + 7;
        char line[64];
        memset(line, ' ', sizeof line);
        char b[16];
        snprintf(b, sizeof b, "%2d", i); memcpy(line, b, 2);
        int l = (int)strlen(db[i].name); memcpy(line + 3, db[i].name, l > 18 ? 18 : l);
        snprintf(b, sizeof b, "%4d", db[i].points); memcpy(line + 0x15, b, 4);
        line[0x19] = 0;
        text_at(line, db[i].ctrl == 5 ? 14 : 10, DST_PAGE, y, x);
    }
    fill_rect(vpage, VW, VH, 160, 0, 1, 200, 14);
    video_present();
}

/* ------------------------------------------------------------ save files (compatible with the original) */

/* File layout of TOURNAMN.nnn / SEASON.nnn (1000:9ec8, 9d5f, d55d, d05d): description (string[25]), the 64 player
 * records of 24 bytes (string[19] name, u16 points, level, control), the draw (64 x 6 bytes), tournament id, round. */
static void write_state(FILE *f, const char *desc)
{
    slot_write_desc(f, desc);
    for (int i = 1; i <= NPLAYERS; i++) {
        uint8_t r[24];
        memset(r, 0, sizeof r);
        int l = (int)strlen(db[i].name);
        if (l > 19) l = 19;
        r[0] = (uint8_t)l;
        memcpy(r + 1, db[i].name, l);
        r[0x14] = db[i].points & 255; r[0x15] = db[i].points >> 8;
        r[0x16] = db[i].level; r[0x17] = db[i].ctrl;
        fwrite(r, 1, 24, f);
    }
    for (int p = 1; p <= NPLAYERS; p++) fwrite(&bracket[p][1], 1, 6, f);
    fputc(saved_id, f); fputc(saved_round, f);
}

static int read_state(FILE *f)
{
    uint8_t d[SAVE_DESC];
    if (fread(d, 1, SAVE_DESC, f) != SAVE_DESC) return 0;
    for (int i = 1; i <= NPLAYERS; i++) {
        uint8_t r[24];
        if (fread(r, 1, 24, f) != 24) return 0;
        int l = r[0] > 19 ? 19 : r[0];
        memset(db[i].name, 0, sizeof db[i].name);
        memcpy(db[i].name, r + 1, l);
        db[i].points = r[0x14] | r[0x15] << 8;
        db[i].level = r[0x16]; db[i].ctrl = r[0x17];
    }
    memset(bracket, 0, sizeof bracket);
    for (int p = 1; p <= NPLAYERS; p++) if (fread(&bracket[p][1], 1, 6, f) != 6) return 0;
    saved_id = fgetc(f); saved_round = fgetc(f);
    return saved_id >= 1 && saved_id <= 12 && saved_round >= 1 && saved_round <= 6;
}

/* 1000:9ec8 (tournament) / 9d5f (season): choose a slot, ask the description, write the file */
static void save_state_dialog(int season)
{
    const char *base = season ? "SEASON" : "TOURNAMN";
    int idx = dlg_slots(season ? "        SAVE SEASON" : "      SAVE TOURNAMENT", season ? "   DELETE SAVED SEASON" : " DELETE SAVED TOURNAMENT", base, 1);
    if (idx < 0) return;
    char desc[SAVE_DESC + 1];
    memset(desc, 0, sizeof desc);
    if (idx < slot_count(base)) slot_read_desc(base, idx, desc);
    for (int i = (int)strlen(desc); i > 0 && desc[i - 1] == ' '; i--) desc[i - 1] = 0;
    if (!dlg_description(desc, 24)) return;
    FILE *f = slot_open(base, idx, "wb");
    if (!f) return;
    write_state(f, desc);
    fclose(f);
}

/* ------------------------------------------------------------ playing */

static int load_match_players(MatchSetup *ms, int near_i, int far_i, int round, int id)
{
    memset(ms, 0, sizeof *ms);
    ms->doubles = 0;
    ms->human[0] = db[near_i].ctrl != 5;
    ms->human[1] = db[far_i].ctrl != 5;
    ms->ctrl[0] = db[near_i].ctrl; ms->ctrl[1] = db[far_i].ctrl;
    ms->cpu_level[0] = db[near_i].level; ms->cpu_level[1] = db[far_i].level;
    ms->cpu_level[2] = ms->cpu_level[3] = 1;
    ms->best_of_3 = !(tour[id].final5 && round == 6);
    ms->court = tour[id].court;
    OptFile o; opt_from_menus(&o);
    ms->speed = o.o[23];
    snprintf(ms->name1, sizeof ms->name1, "%s", db[near_i].name);
    snprintf(ms->name2, sizeof ms->name2, "%s", db[far_i].name);
    for (int i = 0; i < 2; i++) if (!device_check(ms->ctrl[i])) return 0;
    return 1;
}

/* 1000:9907 - play a match of the draw; returns the db index of the winner. first = near player's index. */
static int play_draw_match(int a_far, int b_near, int round, int id)
{
    MatchSetup ms;
    if (!load_match_players(&ms, b_near, a_far, round, id)) return b_near;
    int winner = matchflow_play(&ms, 0);
    int result = winner == 1 ? b_near : a_far;
    if (!g_quit_match) {
        static const char *lab[7] = { "", "- 1ST. ROUND", "- 2ND. ROUND", "- 3RD. ROUND", "- QUARTERFINALS", "- SEMIFINALS", "- FINAL" };
        hof_offer_save(tour[id].name, lab[round]);
    } else video_fade_out(2);
    video_set_scroll(0, 0);
    return result;
}

static int ask_continue(void)                      /* 1000:0786: C/Enter/Space continue, S save, Q/ESC quit */
{
    key_flush();
    for (;;) {
        if (quit_requested) return 'Q';
        video_wait_vsync();
        int sc = key_pressed_scancode();
        if (!sc) continue;
        if (sc == 0x01 || sc == 0x10) return 'Q';
        if (sc == 0x1f) return 'S';
        if (sc == 0x39 || sc == 0x1c || sc == 0x2e) return 'C';
    }
}

/* 1000:9f55 - the champion */
static void winner_screen(const char *name)
{
    music_stop();
    music_start(2);
    font_select(5);
    Palette pal;
    memset(vpage, 0, sizeof vpage);
    Image *im = img_load_pbm("DATA\\BACKGND\\YOUWIN.PBM");
    if (im) { blit(vpage, VW, VH, im, 0, 0); img_free(im); }
    pal_load("DATA\\PAL\\YOUWIN.PAL", &pal);
    text_outlined("WINNER", 0, 4, DST_PAGE, 5, 3);
    text_outlined(name, 0, 10, DST_PAGE, 0x19, 3);
    show_page(&pal);
    wait_keys_released();
    wait_key_press();
    wait_keys_released();
    music_stop();
    music_start(1);
}

/* 1000:969b */
static int run_tournament(int season, int start_round, int id)
{
    int key = 0, winner = 0;
    tournament_intro(id);
    wait_keys_released();
    wait_key_press();
    if (start_round == 1) {
        memset(bracket, 0, sizeof bracket);
        for (int p = 1; p <= NPLAYERS; p++) bracket[p][1] = ds_u8(0xf6f + p);
    }
    for (int round = start_round; round < 7; round++) {
        int human_played = 0;
        for (int p = 1; p <= (0x40 >> (round - 1)); p += 2) {
            int a = bracket[p][round], b = bracket[p + 1][round];
            if (a && b) {
                if (db[a].ctrl == 5 && db[b].ctrl == 5) winner = simulate(b, a);
                else {
                    font_select(5);
                    versus_screen(a, b, round, id);
                    winner = play_draw_match(b, a, round, id);
                    human_played = 1;
                    if (g_quit_match) { /* the original keeps going with the draw */ }
                }
            }
            db[winner].points += tour[id].pts[round];
            if (round < 6) bracket[((p - 1) >> 1) + 1][round + 1] = winner;
        }
        (void)human_played;
        bracket_screen(round == 6 ? winner : 0, id);
        wait_keys_released();
        for (;;) {
            key = ask_continue();
            if (key == 'Q') {
                Dialog d;
                static const char *dummy;
                (void)dummy;
                dialog_init(&d, "     QUIT TOURNAMENT", 5, 0x1b, 0x60, 0x32);
                dialog_add_button(&d, " @YES ", 1, 0x8a, 0x46);
                dialog_add_button(&d, " @NO  ", 0, 0x8a, 0xbe);
                dialog_add_label(&d, "    Sure?", 0x78, 100);
                font_select(8);
                int r = dialog_run(&d);
                font_select(7);
                if (!r) { key = 'S'; continue; }
            }
            if (key == 'S') {
                saved_id = id; saved_round = round;
                save_state_dialog(season);
                video_fade_out(0);
                bracket_screen(round == 6 ? winner : 0, id);
                key_flush();
                continue;
            }
            break;
        }
        if (key == 'Q' || round == 6) break;
    }
    (void)season;
    if (key == 'C') winner_screen(db[winner].name);
    video_fade_out(2);
    return key == 'Q';
}

int tournament_menu_action(Menu *m, MenuItem *it)
{
    (void)m;
    int id = it->id;
    if (id < 1 || id > 12) return MR_STAY;
    db_init();
    db_sort();
    OptFile o; opt_from_menus(&o);
    /* the human players take the last four places of the draw: P1 = 64 ... P4 = 61 */
    for (int s = 0; s < 4; s++) db[0x40 - s].ctrl = o.o[24 + s];
    font_select(8);
    for (int s = 0; s < 4; s++) {
        if (db[0x40 - s].ctrl == 5) continue;
        if (!device_check(db[0x40 - s].ctrl)) return MR_STAY;
        for (;;) {
            char buf[24] = "";
            char title[48];
            snprintf(title, sizeof title, "     PLAYER %d NAME: ", s + 1);
            Dialog d;
            dialog_init(&d, title, 5, 0x1c, 0x60, 0x32);
            dialog_add_edit(&d, buf, 20, 20, 0x77, 0x50);
            dialog_add_button(&d, " @OK ", 1, 0x8a, 0x46);
            dialog_add_button(&d, "@CANCEL", 0, 0x8a, 0xbe);
            if (!dialog_run(&d)) return MR_STAY;
            for (char *c = buf; *c; c++) *c = (char)toupper((unsigned char)*c);
            int dup = 0;
            for (int i = 1; i <= NPLAYERS && !dup; i++) {
                if (i == 0x40 - s) continue;
                int l = db_len(db[i].name);
                char t[24]; snprintf(t, sizeof t, "%.*s", l, db[i].name);
                if (strstr(buf, t)) dup = 1;
            }
            if (dup) { message_box("  ", "That name already exists!", NULL); continue; }
            snprintf(db[0x40 - s].name, sizeof db[0x40 - s].name, "%-18.18s", buf);
            break;
        }
    }
    video_fade_out(2);
    run_tournament(0, 1, id);
    ui_dirty = 1;
    return MR_STAY;
}

/* 1000:d55d */
int tournament_load_action(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    int idx = dlg_slots("      LOAD TOURNAMENT", " DELETE SAVED TOURNAMENT", "TOURNAMN", 0);
    if (idx < 0) { ui_dirty = 1; return MR_STAY; }
    FILE *f = slot_open("TOURNAMN", idx, "rb");
    if (!f) return MR_STAY;
    int ok = read_state(f);
    fclose(f);
    if (!ok) return MR_STAY;
    video_fade_out(2);
    font_select(7);
    run_tournament(0, saved_round, saved_id);   /* the original restarts the saved round */
    ui_dirty = 1;
    return MR_STAY;
}

/* 1000:ce2c - tournaments 'first'..12 one after the other, ranking shown between them */
static void season_run(int first, int start_round)
{
    for (int id = first; id <= 12; id++) {
        int quit = run_tournament(1, id == first ? start_round : 1, id);
        font_select(7);
        db_sort();
        if (quit) break;
        ranking_screen();
        wait_keys_released();
        wait_key_press();
    }
}

int season_load_action(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    int idx = dlg_slots("        LOAD SEASON", "   DELETE SAVED SEASON", "SEASON", 0);
    if (idx < 0) { ui_dirty = 1; return MR_STAY; }
    FILE *f = slot_open("SEASON", idx, "rb");
    if (!f) return MR_STAY;
    int ok = read_state(f);
    fclose(f);
    if (!ok) return MR_STAY;
    video_fade_out(2);
    font_select(7);
    season_run(saved_id, saved_round);
    video_fade_out(2);
    font_select(5);
    ui_dirty = 1;
    return MR_STAY;
}

int season_new_action(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    db_init();
    db_sort();
    OptFile o; opt_from_menus(&o);
    for (int s = 0; s < 4; s++) db[0x40 - s].ctrl = o.o[24 + s];
    font_select(8);
    for (int s = 0; s < 4; s++) {
        if (db[0x40 - s].ctrl == 5) continue;
        if (!device_check(db[0x40 - s].ctrl)) return MR_STAY;
        char buf[24] = "";
        char title[48];
        snprintf(title, sizeof title, "     PLAYER %d NAME: ", s + 1);
        Dialog d;
        dialog_init(&d, title, 5, 0x1c, 0x60, 0x32);
        dialog_add_edit(&d, buf, 20, 20, 0x77, 0x50);
        dialog_add_button(&d, " @OK ", 1, 0x8a, 0x46);
        dialog_add_button(&d, "@CANCEL", 0, 0x8a, 0xbe);
        if (!dialog_run(&d)) return MR_STAY;
        for (char *c = buf; *c; c++) *c = (char)toupper((unsigned char)*c);
        snprintf(db[0x40 - s].name, sizeof db[0x40 - s].name, "%-18.18s", buf);
    }
    video_fade_out(2);
    font_select(7);
    ranking_screen();
    wait_key_press();
    season_run(1, 1);
    video_fade_out(2);
    font_select(5);
    ui_dirty = 1;
    return MR_STAY;
}
