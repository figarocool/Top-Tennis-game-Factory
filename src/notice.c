/* Shown instead of the game when TENNIS.DAT is not where it should be. The game fonts live in that file, so this
 * uses the small bitmap font built into the program. */
#include "screens.h"
#include "video.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

extern const uint8_t minifont[95][12];

static void put_text(const char *s, int y, int color)
{
    int x = (320 - (int)strlen(s) * 8) / 2;
    if (x < 0) x = 0;
    for (; *s; s++, x += 8) {
        int c = (unsigned char)*s;
        if (c < 32 || c > 126) continue;
        for (int j = 0; j < 12; j++)
            for (int i = 0; i < 8; i++)
                if (minifont[c - 32][j] >> (7 - i) & 1 && x + i < VW && y + j < VH) vpage[(y + j) * VW + x + i] = (uint8_t)color;
    }
}

void notice_missing_data(const char *dir, const char *reason)
{
    if (plat_init() || video_init()) return;
    Palette p;
    memset(&p, 0, sizeof p);
    p.c[15][0] = p.c[15][1] = p.c[15][2] = 63;                  /* white */
    p.c[14][0] = 63; p.c[14][1] = 56; p.c[14][2] = 0;           /* yellow */
    memset(vpage, 0, sizeof vpage);
    char path[256];
    snprintf(path, sizeof path, "%.36s", dir);
    put_text(reason, 8, 14);
    put_text("Use files from your original game:", 30, 15);
    put_text("TENNIS.EXE + TENNIS.DAT", 46, 14);
    put_text("Copy both files into:", 62, 15);
    put_text(path, 78, 14);
    put_text("Or prepare DAT using prepare_data.py", 102, 15);
    put_text("Oppure usa il DAT preparato", 124, 14);
    put_text("con tools/prepare_data.py sul PC.", 140, 15);
    put_text("Press any button to exit", 176, 15);
    video_set_scroll(0, 0);
    video_set_split(200);
    video_set_palette(&p);
    key_flush();
    for (int t = 0; t < 70 * 60 && !quit_requested; t++) {         /* a minute at most */
        video_wait_vsync();
        if (key_pressed_scancode()) break;
    }
    video_shutdown();
}
