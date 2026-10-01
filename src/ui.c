#include "ui.h"
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

static const char *shot_list[12] = {
    "SHOT  @1  A/SHOT  @1  B/SHOT  @1  C/SHOT  @1  D/SHOT  @1  E/SHOT  @1  F/SHOT  @1  G/SHOT  @1  H/SHOT  @1  I/SHOT  @1  J/SHOT  @1  K/SHOT  @1  L/SHOT  @1  ?",
    "SHOT  @2  A/SHOT  @2  B/SHOT  @2  C/SHOT  @2  D/SHOT  @2  E/SHOT  @2  F/SHOT  @2  G/SHOT  @2  H/SHOT  @2  I/SHOT  @2  J/SHOT  @2  K/SHOT  @2  L/SHOT  @2  ?",
    "SHOT  @3  A/SHOT  @3  B/SHOT  @3  C/SHOT  @3  D/SHOT  @3  E/SHOT  @3  F/SHOT  @3  G/SHOT  @3  H/SHOT  @3  I/SHOT  @3  J/SHOT  @3  K/SHOT  @3  L/SHOT  @3  ?",
    "SHOT  @4  A/SHOT  @4  B/SHOT  @4  C/SHOT  @4  D/SHOT  @4  E/SHOT  @4  F/SHOT  @4  G/SHOT  @4  H/SHOT  @4  I/SHOT  @4  J/SHOT  @4  K/SHOT  @4  L/SHOT  @4  ?",
    "SHOT  @5  A/SHOT  @5  B/SHOT  @5  C/SHOT  @5  D/SHOT  @5  E/SHOT  @5  F/SHOT  @5  G/SHOT  @5  H/SHOT  @5  I/SHOT  @5  J/SHOT  @5  K/SHOT  @5  L/SHOT  @5  ?",
    "SHOT  @6  A/SHOT  @6  B/SHOT  @6  C/SHOT  @6  D/SHOT  @6  E/SHOT  @6  F/SHOT  @6  G/SHOT  @6  H/SHOT  @6  I/SHOT  @6  J/SHOT  @6  K/SHOT  @6  L/SHOT  @6  ?",
    "SHOT  @7  A/SHOT  @7  B/SHOT  @7  C/SHOT  @7  D/SHOT  @7  E/SHOT  @7  F/SHOT  @7  G/SHOT  @7  H/SHOT  @7  I/SHOT  @7  J/SHOT  @7  K/SHOT  @7  L/SHOT  @7  ?",
    "SHOT  @8  A/SHOT  @8  B/SHOT  @8  C/SHOT  @8  D/SHOT  @8  E/SHOT  @8  F/SHOT  @8  G/SHOT  @8  H/SHOT  @8  I/SHOT  @8  J/SHOT  @8  K/SHOT  @8  L/SHOT  @8  ?",
    "SHOT  @9  A/SHOT  @9  B/SHOT  @9  C/SHOT  @9  D/SHOT  @9  E/SHOT  @9  F/SHOT  @9  G/SHOT  @9  H/SHOT  @9  I/SHOT  @9  J/SHOT  @9  K/SHOT  @9  L/SHOT  @9  ?",
    "SHOT 1@0  A/SHOT 1@0  B/SHOT 1@0  C/SHOT 1@0  D/SHOT 1@0  E/SHOT 1@0  F/SHOT 1@0  G/SHOT 1@0  H/SHOT 1@0  I/SHOT 1@0  J/SHOT 1@0  K/SHOT 1@0  L/SHOT 1@0  ?",
    "@SHOT 11  A/@SHOT 11  B/@SHOT 11  C/@SHOT 11  D/@SHOT 11  E/@SHOT 11  F/@SHOT 11  G/@SHOT 11  H/@SHOT 11  I/@SHOT 11  J/@SHOT 11  K/@SHOT 11  L/@SHOT 11  ?",
    "S@HOT 12  A/S@HOT 12  B/S@HOT 12  C/S@HOT 12  D/S@HOT 12  E/S@HOT 12  F/S@HOT 12  G/S@HOT 12  H/S@HOT 12  I/S@HOT 12  J/S@HOT 12  K/S@HOT 12  L/S@HOT 12  ?",
};

void ui_build(void)
{
    /* tournament list: 12 cities, LOAD TOURNAMENT, CANCEL */
    static const char *cities[12] = { "@SYDNEY", "@MELBOURNE", "S@AN FRANCISCO", "@TOKYO", "@BARCELONA", "@PARIS",
                                      "LON@DON", "M@ONTREAL", "@NEW YORK", "STOC@KHOLM", "MOSCO@W", "@FRANKFURT" };
    menu_init(&menu_tour, "DATA\\BACKGND\\MENUTOUR.PBM", "DATA\\PAL\\MENUTOUR.PAL", 1, 0x58);
    for (int i = 0; i < 12; i++) menu_add_command(&menu_tour, cities[i], act_tournament_city, NULL)->id = i + 1;
    menu_add_command(&menu_tour, "@LOAD TOURNAMENT", act_load_tournament, NULL);
    menu_add_command(&menu_tour, "@CANCEL", menu_close_action, NULL);

    menu_init(&menu_season, "DATA\\BACKGND\\MENUSEAS.PBM", "DATA\\PAL\\MENUSEAS.PAL", 0x5f, 0x5a);
    menu_add_command(&menu_season, "@NEW SEASON", act_new_season, NULL);
    menu_add_command(&menu_season, "@LOAD SEASON", act_load_season, NULL);
    menu_add_command(&menu_season, "@CANCEL", menu_close_action, NULL);

    menu_init(&menu_play, "DATA\\BACKGND\\MENUPLA2.PBM", "DATA\\PAL\\MENUPLAY.PAL", 0x23, 0x32);
    menu_add_command(&menu_play, "@FRIENDLY MATCH", act_friendly, NULL);
    menu_add_command(&menu_play, "@TOURNAMENT", NULL, &menu_tour);
    menu_add_command(&menu_play, "FULL @SEASON", NULL, &menu_season);
    menu_add_choice(&menu_play, "@NUMBER .......... SINGLES/@NUMBER .......... DOUBLES", 0);
    menu_add_choice(&menu_play, "@CPU LEVEL ...... BEGINNER/@CPU LEVEL ....... AMATEUR/@CPU LEVEL ........... PRO/@CPU LEVEL ........... TOP", 0);
    menu_add_choice(&menu_play, "MATCH @LENGTH ..... 3 SETS/MATCH @LENGTH ..... 5 SETS", 0);
    menu_add_choice(&menu_play, "C@OURT TYPE ......... CLAY/C@OURT TYPE ......... HARD/C@OURT TYPE ........ GRASS/C@OURT TYPE ....... INDOOR", 0);
    menu_add_command(&menu_play, "LOAD @REPLAY", act_load_replay, NULL);
    menu_add_command(&menu_play, "@HALL OF FAME", act_hall_of_fame, NULL);
    menu_add_command(&menu_play, "@MAIN MENU", menu_close_action, NULL);

    menu_init(&menu_machine, "DATA\\BACKGND\\MENUMACH.PBM", "DATA\\PAL\\MENUMACH.PAL", 1, 0xb);
    for (int i = 0; i < 12; i++) menu_add_choice(&menu_machine, shot_list[i], i);
    menu_add_command(&menu_machine, "C@ONTINUE", act_machine_continue, NULL);
    menu_add_command(&menu_machine, "@CANCEL", menu_close_action, NULL);

    menu_init(&menu_training, "DATA\\BACKGND\\MENUTRAI.PBM", "DATA\\PAL\\MENUTRAI.PAL", 0x34, 0x32);
    menu_add_command(&menu_training, "@SERVE TRAINING", act_serve_training, NULL);
    menu_add_command(&menu_training, "M@ACHINE TRAINING", NULL, &menu_machine);
    menu_add_choice(&menu_training, "@PLAYER SIDE ......... UP/@PLAYER SIDE ....... DOWN", 1);
    menu_add_choice(&menu_training, "SERVE P@OSITION ... FIXED/SERVE P@OSITION  ROTATIVE", 0);
    menu_add_choice(&menu_training, "MACHINE @DELAY .... SHORT/MACHINE @DELAY ... MEDIUM/MACHINE @DELAY ..... LONG", 0);
    menu_add_choice(&menu_training, "@COURT TYPE ........ CLAY/@COURT TYPE ........ HARD/@COURT TYPE ....... GRASS/@COURT TYPE ...... INDOOR", 0);
    menu_add_command(&menu_training, "@MAIN MENU", menu_close_action, NULL);

    menu_init(&menu_options, "DATA\\BACKGND\\MENUOPT.PBM", "DATA\\PAL\\MENUOPT.PAL", 1, 0x3a);
    menu_add_choice(&menu_options, "M@USIC VOLUME ..... OFF/M@USIC VOLUME ..... 25%/M@USIC VOLUME ..... 50%/M@USIC VOLUME ..... 75%/M@USIC VOLUME .... 100%", 4);
    menu_add_choice(&menu_options, "SOUND @FX VOLUME .. OFF/SOUND @FX VOLUME .. 25%/SOUND @FX VOLUME .. 50%/SOUND @FX VOLUME .. 75%/SOUND @FX VOLUME . 100%", 4);
    menu_add_choice(&menu_options, "@GAME SPEED ....... LOW/@GAME SPEED .... NORMAL/@GAME SPEED ...... HIGH", 1);
    static const char *pl[4] = {
        "PLAYER @1 .. KEYBOARD 1/PLAYER @1 .. KEYBOARD 2/PLAYER @1 .. JOYSTICK 1/PLAYER @1 .. JOYSTICK 2/PLAYER @1 ......... CPU",
        "PLAYER @2 .. KEYBOARD 1/PLAYER @2 .. KEYBOARD 2/PLAYER @2 .. JOYSTICK 1/PLAYER @2 .. JOYSTICK 2/PLAYER @2 ......... CPU",
        "PLAYER @3 .. KEYBOARD 1/PLAYER @3 .. KEYBOARD 2/PLAYER @3 .. JOYSTICK 1/PLAYER @3 .. JOYSTICK 2/PLAYER @3 ......... CPU",
        "PLAYER @4 .. KEYBOARD 1/PLAYER @4 .. KEYBOARD 2/PLAYER @4 .. JOYSTICK 1/PLAYER @4 .. JOYSTICK 2/PLAYER @4 ......... CPU" };
    static const int pdef[4] = { 0, 4, 4, 4 };       /* values of the shipped TENNIS.OPT */
    for (int i = 0; i < 4; i++) menu_add_choice(&menu_options, pl[i], pdef[i]);
    menu_add_command(&menu_options, "@REDEFINE KEYBOARD 1", act_redefine_kb1, NULL);
    menu_add_command(&menu_options, "R@EDEFINE KEYBOARD 2", act_redefine_kb2, NULL);
    menu_add_command(&menu_options, "@CALIBRATE JOYSTICK 1", act_calibrate_j1, NULL);
    menu_add_command(&menu_options, "C@ALIBRATE JOYSTICK 2", act_calibrate_j2, NULL);
    menu_add_command(&menu_options, "@SAVE OPTIONS", act_save_options, NULL);
    menu_add_command(&menu_options, "@LOAD OPTIONS", act_load_options, NULL);
    menu_add_command(&menu_options, "@MAIN MENU", menu_close_action, NULL);

    menu_init(&menu_main, "DATA\\BACKGND\\MENUMAI2.PBM", "DATA\\PAL\\MENUMAIN.PAL", 0x58, 0x7c);
    menu_add_command(&menu_main, "@PLAY", NULL, &menu_play);
    menu_add_command(&menu_main, "@DEMO", act_demo, NULL);
    menu_add_command(&menu_main, "@TRAINING", NULL, &menu_training);
    menu_add_command(&menu_main, "@NETWORK", act_network, NULL);
    menu_add_command(&menu_main, "@OPTIONS", NULL, &menu_options);
    menu_add_command(&menu_main, "@CREDITS", act_credits, NULL);
    menu_add_command(&menu_main, "@EXIT", act_exit, NULL);
}

/* 1000:a0e0 / 1008:086b - the volume selections of the OPTIONS menu drive the sound card mixer */
void ui_apply_options(void)
{
    static const int pct[5] = { 0, 20, 47, 73, 100 };       /* mixer levels 0,3,7,11,15 of 15 */
    int sfx = menu_choice(&menu_options, OPT_SFX);
    if (sfx >= 1 && sfx <= 5) { snd_set_volume(pct[sfx - 1]); snd_set_enabled(sfx > 1); }
    int mus = menu_choice(&menu_options, OPT_MUSIC);
    if (mus >= 1 && mus <= 5) {
        static int last = -1;
        snd_set_music(pct[mus - 1], mus > 1);
        if (mus != last) { last = mus; if (mus > 1) music_start(1); }
    }
}
