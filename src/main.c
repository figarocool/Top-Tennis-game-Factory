/* Top Tennis - C port. Usage: toptennis [data dir containing TENNIS.DAT] */
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
#include "dsimg.h"
#include "pak.h"

int main(int argc, char **argv)
{
#ifdef __PSP__
    static char psp_dir[256];                          /* TENNIS.DAT sits next to EBOOT.PBP */
    snprintf(psp_dir, sizeof psp_dir, "%s", argc > 0 && argv[0] ? argv[0] : "ms0:/PSP/GAME/TopTennis/EBOOT.PBP");
    char *slash = strrchr(psp_dir, '/');
    if (slash) *slash = 0; else snprintf(psp_dir, sizeof psp_dir, "ms0:/PSP/GAME/TopTennis");
    const char *dir = psp_dir;
#elif defined(__vita__)
    const char *dir = "ux0:data/TopTennis";           /* copy TENNIS.DAT of the original game here */
    (void)argc; (void)argv;
#else
    const char *dir = argc > 1 ? argv[1] : "orig";
#endif
    snprintf(data_dir, sizeof data_dir, "%s", dir);
    {   /* test hook for consoles/emulators without environment variables: <dir>/tt_input.txt holds the TT_INPUT script */
        char tp[600], txt[2048];
        snprintf(tp, sizeof tp, "%s/tt_input.txt", dir);
        FILE *tf = fopen(tp, "r");
        if (tf) {
            size_t n = fread(txt, 1, sizeof txt - 1, tf);
            txt[n] = 0;
            while (n && (txt[n - 1] == '\n' || txt[n - 1] == '\r')) txt[--n] = 0;
            fclose(tf);
            setenv("TT_INPUT", txt, 1);
        }
        snprintf(tp, sizeof tp, "%s/tt_env.txt", dir);       /* more test variables, one NAME=value per line */
        tf = fopen(tp, "r");
        if (tf) {
            char line[256];
            while (fgets(line, sizeof line, tf)) {
                char *eq = strchr(line, '=');
                if (!eq) continue;
                *eq = 0;
                char *v = eq + 1;
                v[strcspn(v, "\r\n")] = 0;
                setenv(line, v, 1);
            }
            fclose(tf);
        }
    }
#if defined(__vita__) || defined(__PSP__)
    { char lp[600]; snprintf(lp, sizeof lp, "%s/log.txt", dir); freopen(lp, "w", stderr); }        /* for diagnosing problems on the console */
    setvbuf(stderr, NULL, _IONBF, 0);
    fprintf(stderr, "Top Tennis starting\n");
#endif
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
        dialog_init(&d, ds_cstr(0x30f6), 5, 0x1c, 0x60, 0x32);
        dialog_add_edit(&d, name, 20, 20, 0x77, 0x50);
        dialog_add_button(&d, ds_cstr(0x3057), 1, 0x8a, 0x46);
        dialog_add_button(&d, ds_cstr(0x1c2c), 0, 0x8a, 0xbe);
        font_select(8);
        dialog_run(&d);
        return 0;
    }
    menu_run(start);
    video_shutdown();
    plat_quit();
    pak_close();
    ds_unload();
    return 0;
}
