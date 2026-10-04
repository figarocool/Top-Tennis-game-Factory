#include "dsimg.h"
#include "screens.h"
#include "text.h"
#include "video.h"
#include "platform.h"
#include <string.h>

static int any_key_held(void)
{
    plat_poll();
    for (int i = 1; i < 256; i++) if (keys[i]) return 1;
    return 0;
}

int wait_vsyncs_or_key(int n)
{
    for (int i = 0; i < n; i++) {
        if (any_key_held() || quit_requested) return 1;
        video_wait_vsync();
    }
    return 0;
}

void screen_show_image(const char *pbm)
{
    Image *im = img_load_pbm(pbm);
    if (!im) return;
    memset(vpage, 0, sizeof vpage);
    blit(vpage, VW, VH, im, 0, 0);
    img_free(im);
    video_set_scroll(0, 0);
    video_set_split(200);
}

/* 1000:e520 */
/* 1000:e660 / e6b8 - "LOADING ... PLEASE WAIT" with a progress bar that grows 3 pixels per loaded group of assets.
 * Loading is instant here, so the bar is paced by a timer (68 steps of 3 vsyncs, about 3 seconds). */
static void loading_screen(void)
{
    Palette pal;
    memset(vpage, 0, sizeof vpage);
    pal_load("DATA\\PAL\\MENUMAIN.PAL", &pal);       /* the palette set just before the fonts are loaded (384d) */
    video_set_palette(&pal);
    font_select(5);
    text_at(ds_cstr(0x31b2), 10, DST_PAGE, 0x8c, 0x37);
    fill_rect(vpage, VW, VH, 0x37, 0x9b, 0x105 - 0x37 + 1, 0xa6 - 0x9b + 1, 14);
    fill_rect(vpage, VW, VH, 0x38, 0x9c, 0x104 - 0x38 + 1, 0xa5 - 0x9c + 1, 8);
    video_present();
    int x = 0x38;
    key_flush();
    for (int step = 0; step < 68; step++) {
        fill_rect(vpage, VW, VH, x, 0x9c, 3, 0xa5 - 0x9c + 1, 4);
        x += 3;
        for (int i = 0; i < 3; i++) {
            video_wait_vsync();
            if (quit_requested) return;
        }
        if (key_pressed_scancode()) { fill_rect(vpage, VW, VH, x, 0x9c, 0x104 - x + 1, 0xa5 - 0x9c + 1, 4); video_present(); return; }
    }
}

void screen_intro(void)
{
    Palette pal;
    video_black();
    screen_show_image("DATA\\BACKGND\\TGFLOGO.PBM");
    pal_load("DATA\\PAL\\TGFLOGO.PAL", &pal);
    /* the "presents" line is hidden for the first part (black rectangle x 115..191, y 152..179) */
    static uint8_t save[VW * 30];
    for (int y = 152; y < 180; y++) memcpy(save + (y - 152) * VW, vpage + y * VW, VW);
    fill_rect(vpage, VW, VH, 0x73, 0x98, 0xc0 - 0x73, 0xb4 - 0x98, 0);
    video_fade_in(&pal, 2);
    if (!wait_vsyncs_or_key(140)) {
        for (int y = 152; y < 180; y++) memcpy(vpage + y * VW, save + (y - 152) * VW, VW);
        wait_vsyncs_or_key(140);
    }
    video_fade_out(2);
    screen_show_image("DATA\\BACKGND\\PRESENT2.PBM");
    pal_load("DATA\\PAL\\PRESENT2.PAL", &pal);
    video_set_palette(&pal);
    wait_vsyncs_or_key(210);
    video_fade_out(2);
    loading_screen();
    video_fade_out(2);
}
