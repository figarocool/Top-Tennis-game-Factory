/* Top Tennis - C port. Usage: toptennis [data dir containing TENNIS.DAT and TENNIS.EXE] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gamecfg.h"
#include "game.h"
#include "video.h"
#include "menu.h"
#include "ui.h"
#include "options.h"
#include "screens.h"
#include "dialog.h"
#include "text.h"
#include "hof.h"
#include "sound.h"
#include "platform.h"

int main(int argc, char **argv)
{
#ifdef __vita__
    const char *dir = "ux0:data/TopTennis";           /* copy TENNIS.DAT and TENNIS.EXE of the original game here */
    (void)argc; (void)argv;
#else
    const char *dir = argc > 1 ? argv[1] : "orig";
#endif
    snprintf(data_dir, sizeof data_dir, "%s", dir);
    if (game_boot(dir)) return 1;
    cursor_init();
    opt_defaults();
    ui_build();
    opt_load();
    hof_load();
    if (!getenv("TT_MENU")) screen_intro();

    ui_apply_options();
    Menu *start = &menu_main;
    const char *tm = getenv("TT_MENU");
    if (tm) {
        if (!strcmp(tm, "play")) start = &menu_play;
        else if (!strcmp(tm, "tour")) start = &menu_tour;
        else if (!strcmp(tm, "season")) start = &menu_season;
        else if (!strcmp(tm, "training")) start = &menu_training;
        else if (!strcmp(tm, "machine")) start = &menu_machine;
        else if (!strcmp(tm, "options")) start = &menu_options;
    }
    if (getenv("TT_DLG")) {              /* test: the "PLAYER 1 NAME:" dialog over the play menu */
        menu_run_once_draw(&menu_play);
        static char name[32] = "";
        Dialog d;
        dialog_init(&d, "     PLAYER 1 NAME:", 5, 0x1c, 0x60, 0x32);
        dialog_add_edit(&d, name, 20, 20, 0x77, 0x50);
        dialog_add_button(&d, " @OK ", 1, 0x8a, 0x46);
        dialog_add_button(&d, "@CANCEL", 0, 0x8a, 0xbe);
        font_select(8);
        dialog_run(&d);
        return 0;
    }
    menu_run(start);
    video_shutdown();
    plat_quit();
    return 0;
}
