#include "dsimg.h"
#include "ui.h"
#include "video.h"
#include "joy.h"
#include "sound.h"
#include <stdio.h>
#include <string.h>

Menu menu_main, menu_play, menu_tour, menu_season, menu_machine, menu_training, menu_options;

int menu_choice(Menu *m, int idx1)
{
    if (idx1 < 1 || idx1 > m->n) return 0;
    return m->items[idx1 - 1].sel + 1;
}

void menu_set_choice(Menu *m, int idx1, int sel1)
{
    if (idx1 < 1 || idx1 > m->n) return;
    MenuItem *it = &m->items[idx1 - 1];
    if (sel1 >= 1 && sel1 <= it->nchoices) it->sel = sel1 - 1;
}

int tournament_menu_action(Menu *m, MenuItem *it); int season_new_action(Menu *m, MenuItem *it);
#define act_tournament_city tournament_menu_action
int tournament_load_action(Menu *m, MenuItem *it);
#define act_load_tournament tournament_load_action
#define act_new_season season_new_action
/* actions are provided by the game modules (weak stubs keep the menus usable on their own) */
#define STUB(name) __attribute__((weak)) int name(Menu *m, MenuItem *it) { (void)m; (void)it; return MR_STAY; }
STUB(act_friendly)
int season_load_action(Menu *m, MenuItem *it);
#define act_load_season season_load_action
int act_load_replay(Menu *m, MenuItem *it);
int act_hall_of_fame(Menu *m, MenuItem *it);
int act_demo(Menu *m, MenuItem *it);
int act_network(Menu *m, MenuItem *it);
int act_serve_training(Menu *m, MenuItem *it); int act_machine_continue(Menu *m, MenuItem *it);

int act_exit(Menu *m, MenuItem *it); int act_credits(Menu *m, MenuItem *it); int act_save_options(Menu *m, MenuItem *it);
int act_load_options(Menu *m, MenuItem *it); int act_redefine_kb1(Menu *m, MenuItem *it); int act_redefine_kb2(Menu *m, MenuItem *it);

static const unsigned shot_offsets[12] = {
    0x1e2e,
    0x1eca,
    0x1f66,
    0x2002,
    0x209e,
    0x213a,
    0x21d6,
    0x2272,
    0x230e,
    0x23aa,
    0x2446,
    0x24e2,
};

void ui_build(void)
{
    /* tournament list: 12 cities, LOAD TOURNAMENT, CANCEL */
    const char *cities[12] = { ds_cstr(0x1ba6), ds_cstr(0x1bae), ds_cstr(0x1bb9), ds_cstr(0x1bc8), ds_cstr(0x1bcf), ds_cstr(0x1bda),
                                      ds_cstr(0x1be1), ds_cstr(0x1be9), ds_cstr(0x1bf3), ds_cstr(0x1bfd), ds_cstr(0x1c08), ds_cstr(0x1c10) };
    menu_init(&menu_tour, "DATA\\BACKGND\\MENUTOUR.PBM", "DATA\\PAL\\MENUTOUR.PAL", 1, 0x58);
    for (int i = 0; i < 12; i++) menu_add_command(&menu_tour, cities[i], act_tournament_city, NULL)->id = i + 1;
    menu_add_command(&menu_tour, ds_cstr(0x1c1b), act_load_tournament, NULL);
    menu_add_command(&menu_tour, ds_cstr(0x1c2c), menu_close_action, NULL);

    menu_init(&menu_season, "DATA\\BACKGND\\MENUSEAS.PBM", "DATA\\PAL\\MENUSEAS.PAL", 0x5f, 0x5a);
    menu_add_command(&menu_season, ds_cstr(0x1c4e), act_new_season, NULL);
    menu_add_command(&menu_season, ds_cstr(0x1c5a), act_load_season, NULL);
    menu_add_command(&menu_season, ds_cstr(0x1c2c), menu_close_action, NULL);

    menu_init(&menu_play, "DATA\\BACKGND\\MENUPLA2.PBM", "DATA\\PAL\\MENUPLAY.PAL", 0x23, 0x32);
    menu_add_command(&menu_play, ds_cstr(0x1c81), act_friendly, NULL);
    menu_add_command(&menu_play, ds_cstr(0x1c91), NULL, &menu_tour);
    menu_add_command(&menu_play, ds_cstr(0x1c9d), NULL, &menu_season);
    menu_add_choice(&menu_play, ds_cstr(0x1caa), 0);
    menu_add_choice(&menu_play, ds_cstr(0x1ce0), 0);
    menu_add_choice(&menu_play, ds_cstr(0x1d4c), 0);
    menu_add_choice(&menu_play, ds_cstr(0x1d82), 0);
    menu_add_command(&menu_play, ds_cstr(0x1dee), act_load_replay, NULL);
    menu_add_command(&menu_play, ds_cstr(0x1dfb), act_hall_of_fame, NULL);
    menu_add_command(&menu_play, ds_cstr(0x1e09), menu_close_action, NULL);

    menu_init(&menu_machine, "DATA\\BACKGND\\MENUMACH.PBM", "DATA\\PAL\\MENUMACH.PAL", 1, 0xb);
    for (int i = 0; i < 12; i++) menu_add_choice(&menu_machine, ds_cstr(shot_offsets[i]), i);
    menu_add_command(&menu_machine, ds_cstr(0x257e), act_machine_continue, NULL);
    menu_add_command(&menu_machine, ds_cstr(0x1c2c), menu_close_action, NULL);

    menu_init(&menu_training, "DATA\\BACKGND\\MENUTRAI.PBM", "DATA\\PAL\\MENUTRAI.PAL", 0x34, 0x32);
    menu_add_command(&menu_training, ds_cstr(0x25a2), act_serve_training, NULL);
    menu_add_command(&menu_training, ds_cstr(0x25b2), NULL, &menu_machine);
    menu_add_choice(&menu_training, ds_cstr(0x25c4), 1);
    menu_add_choice(&menu_training, ds_cstr(0x25f8), 0);
    menu_add_choice(&menu_training, ds_cstr(0x262c), 0);
    menu_add_choice(&menu_training, ds_cstr(0x267a), 0);
    menu_add_command(&menu_training, ds_cstr(0x1e09), menu_close_action, NULL);

    menu_init(&menu_options, "DATA\\BACKGND\\MENUOPT.PBM", "DATA\\PAL\\MENUOPT.PAL", 1, 0x3a);
    menu_add_choice(&menu_options, ds_cstr(0x26fb), 4);
    menu_add_choice(&menu_options, ds_cstr(0x2773), 4);
    menu_add_choice(&menu_options, ds_cstr(0x27eb), 1);
    const char *pl[4] = {
        ds_cstr(0x2833),
        ds_cstr(0x28ab),
        ds_cstr(0x2923),
        ds_cstr(0x299b) };
    static const int pdef[4] = { 0, 4, 4, 4 };       /* values of the shipped TENNIS.OPT */
    for (int i = 0; i < 4; i++) menu_add_choice(&menu_options, pl[i], pdef[i]);
#if defined(__vita__) || defined(__PSP__)
    /* no keyboard to redefine and no analog game port to calibrate: the room is used for the screen format */
    menu_add_choice(&menu_options, "SCR@EEN FORMAT .... 4:3/SCR@EEN FORMAT ... 16:9", g_aspect - 1);
#else
    menu_add_command(&menu_options, ds_cstr(0x2a13), act_redefine_kb1, NULL);
    menu_add_command(&menu_options, ds_cstr(0x2a28), act_redefine_kb2, NULL);
    menu_add_command(&menu_options, ds_cstr(0x2a3d), act_calibrate_j1, NULL);
    menu_add_command(&menu_options, ds_cstr(0x2a53), act_calibrate_j2, NULL);
#endif
    menu_add_command(&menu_options, ds_cstr(0x2a69), act_save_options, NULL);
    menu_add_command(&menu_options, ds_cstr(0x2a77), act_load_options, NULL);
    menu_add_command(&menu_options, ds_cstr(0x1e09), menu_close_action, NULL);

    menu_init(&menu_main, "DATA\\BACKGND\\MENUMAI2.PBM", "DATA\\PAL\\MENUMAIN.PAL", 0x58, 0x7c);
    menu_add_command(&menu_main, ds_cstr(0x2a9f), NULL, &menu_play);
    menu_add_command(&menu_main, ds_cstr(0x2aa5), act_demo, NULL);
    menu_add_command(&menu_main, ds_cstr(0x2aab), NULL, &menu_training);
    menu_add_command(&menu_main, "@NETWORK", act_network, NULL);
    menu_add_command(&menu_main, ds_cstr(0x2ab5), NULL, &menu_options);
    menu_add_command(&menu_main, ds_cstr(0x2abe), act_credits, NULL);
    menu_add_command(&menu_main, ds_cstr(0x2ac7), act_exit, NULL);
}

/* 1000:a0e0 / 1008:086b - the volume selections of the OPTIONS menu drive the sound card mixer */
void ui_apply_options(void)
{
    static const int pct[5] = { 0, 20, 47, 73, 100 };       /* mixer levels 0,3,7,11,15 of 15 */
    int sfx = menu_choice(&menu_options, OPT_SFX);
    if (sfx >= 1 && sfx <= 5) { snd_set_volume(pct[sfx - 1]); snd_set_enabled(sfx > 1); }
#if defined(__vita__) || defined(__PSP__)
    { int sc = menu_choice(&menu_options, OPT_SCREEN); if (sc >= 1 && sc <= 2 && sc != g_aspect) video_set_aspect(sc); }
#endif
    int mus = menu_choice(&menu_options, OPT_MUSIC);
    if (mus >= 1 && mus <= 5) {
        static int last = -1;
        snd_set_music(pct[mus - 1], mus > 1);
        if (mus != last) { last = mus; if (mus > 1) music_start(1); }
    }
}
