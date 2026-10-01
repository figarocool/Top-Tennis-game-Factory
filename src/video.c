#include "video.h"
#include "platform.h"
#include <SDL.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

uint8_t vpage[VW * VH];
uint8_t vhud[VW * HUD_H];
Palette vpal;
int     scroll_x, scroll_y;
int     split_line = 200;

static SDL_Window   *win;
static SDL_Renderer *ren;
static SDL_Texture  *tex;
static uint32_t      frame[SCR_W * SCR_H];
static uint32_t      next_vsync;

#define VSYNC_TICKS (PIT_HZ * 1000u / 70086u * 1u)   /* 70.086 Hz */

int g_aspect =
#if defined(__vita__) || defined(__PSP__)
    2;
#else
    1;
#endif

void video_set_aspect(int mode)
{
    g_aspect = mode == 2 ? 2 : 1;
    if (ren) SDL_RenderSetLogicalSize(ren, 960, g_aspect == 2 ? 544 : 720);
}

void video_touch_to_game(float nx, float ny, float *gx, float *gy)
{
    /* the logical screen (960 x 720 or 960 x 544) is fitted into the 960 x 544 display of the Vita */
    float lh = g_aspect == 2 ? 544.0f : 720.0f;
    float scale = 544.0f / lh, xoff = (960.0f - 960.0f * scale) / 2;
    *gx = (nx * 960.0f - xoff) / scale / 3.0f;
    *gy = (ny * 544.0f) / scale * 200.0f / lh;
}

int video_init(void)
{
#ifdef __PSP__
    win = SDL_CreateWindow("Top Tennis", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 272, SDL_WINDOW_FULLSCREEN);
#else
    win = SDL_CreateWindow("Top Tennis", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720,
#ifdef __vita__
                           SDL_WINDOW_FULLSCREEN);
#else
                           SDL_WINDOW_RESIZABLE);
#endif
#endif
    if (!win) return -1;
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) return -1;
    SDL_RenderSetLogicalSize(ren, 960, g_aspect == 2 ? 544 : 720);          /* 320x200 is shown with a 4:3 aspect, like a CRT */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCR_W, SCR_H);
    if (!tex) return -1;
    next_vsync = timer_ticks();
    return 0;
}

void video_shutdown(void)
{
    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    tex = NULL; ren = NULL; win = NULL;
}

void video_toggle_fullscreen(void)
{
    Uint32 f = SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(win, f ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void video_refresh(void)
{
    if (!ren) return;
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

static uint32_t rgb(const uint8_t *c)
{
    /* 6-bit DAC value to 8 bits the way DOSBox/real VGA hardware do: v<<2 | v>>4 */
    return 0xff000000u | (uint32_t)((c[0] << 2) | (c[0] >> 4)) << 16 | (uint32_t)((c[1] << 2) | (c[1] >> 4)) << 8 | (uint32_t)((c[2] << 2) | (c[2] >> 4));
}

void video_present(void)
{
    uint32_t lut[256];
    for (int i = 0; i < 256; i++) lut[i] = rgb(vpal.c[i]);
    int sx = scroll_x, sy = scroll_y;
    if (sx < 0) sx = 0;
    if (sx > VW - SCR_W) sx = VW - SCR_W;
    if (sy < 0) sy = 0;
    if (sy > VH - 1) sy = VH - 1;
    for (int y = 0; y < SCR_H; y++) {
        uint32_t *o = frame + y * SCR_W;
        const uint8_t *s;
        if (y >= split_line && y - split_line < HUD_H)
            s = vhud + (y - split_line) * VW;
        else {
            int py = sy + y;
            if (py >= VH) py = VH - 1;
            s = vpage + py * VW + sx;
        }
        for (int x = 0; x < SCR_W; x++) o[x] = lut[s[x]];
    }
    SDL_UpdateTexture(tex, NULL, frame, SCR_W * 4);
    video_refresh();

    /* test hooks: TT_INPUT="frame:scancode:down|up,..." scripted key events, TT_SHOT_FRAMES="n,n,..." with
     * TT_SHOT=<prefix> saves <prefix><n>.bmp for those frames and quits after the last one */
    static int nframe = 0;
    nframe++;
    plat_script_tick(nframe);
    if (getenv("TT_TRACE") && nframe % 100 == 0) fprintf(stderr, "frame %d\n", nframe);
    const char *shot = getenv("TT_SHOT"), *frs = getenv("TT_SHOT_FRAMES");
    if (shot && frs) {
        int last = 0;
        for (const char *q = frs; *q;) {
            int n = atoi(q);
            if (n > last) last = n;
            if (n == nframe) {
                char name[256];
                snprintf(name, sizeof name, "%s%d.bmp", shot, n);
                SDL_Surface *sf = SDL_CreateRGBSurfaceFrom(frame, SCR_W, SCR_H, 32, SCR_W * 4,
                                                           0xff0000, 0xff00, 0xff, 0xff000000);
                SDL_SaveBMP(sf, name);
                SDL_FreeSurface(sf);
            }
            while (*q && *q != ',') q++;
            if (*q == ',') q++;
        }
        if (nframe >= last) quit_requested = 1;
    }
}

void video_wait_vsync(void)
{
    video_present();
    /* pace to 70.086 Hz without drifting */
    uint32_t now = timer_ticks();
    next_vsync += PIT_HZ * 1000u / 70086u;
    int32_t d = (int32_t)(next_vsync - now);
    if (d < -(int32_t)(PIT_HZ / 10)) next_vsync = now;       /* fell far behind: resync */
    else while ((int32_t)(next_vsync - timer_ticks()) > 1000) {
        plat_poll();
        plat_sleep_ms(1);
    }
    plat_poll();
}

void video_set_palette(const Palette *p) { vpal = *p; }
void video_set_scroll(int y, int x) { scroll_y = y; scroll_x = x; }
void video_set_split(int line) { split_line = line; }

void blit(uint8_t *dst, int dstw, int dsth, const Image *src, int x, int y)
{
    for (int j = 0; j < src->h; j++) {
        int dy = y + j;
        if (dy < 0 || dy >= dsth) continue;
        for (int i = 0; i < src->w; i++) {
            int dx = x + i;
            if (dx < 0 || dx >= dstw) continue;
            if (src->mask && !src->mask[j * src->w + i]) continue;
            dst[dy * dstw + dx] = src->px[j * src->w + i];
        }
    }
}

void fill_rect(uint8_t *dst, int dstw, int dsth, int x, int y, int w, int h, uint8_t c)
{
    for (int j = 0; j < h; j++) {
        int dy = y + j;
        if (dy < 0 || dy >= dsth) continue;
        for (int i = 0; i < w; i++) {
            int dx = x + i;
            if (dx >= 0 && dx < dstw) dst[dy * dstw + dx] = c;
        }
    }
}

void clear_page(uint8_t c) { memset(vpage, c, sizeof vpage); }

int draw_char(uint8_t *dst, int dstw, int dsth, const Font *f, int x, int y, int ch, uint8_t col)
{
    int idx = ch - f->first;
    int adv = font_char_width(f, ch);
    if (idx < 0 || idx >= f->count) return adv;
    const uint8_t *g = f->glyphs + idx * (f->height + (f->advance == 0));
    for (int r = 0; r < f->height; r++) {
        int dy = y + r;
        if (dy < 0 || dy >= dsth) continue;
        for (int b = 0; b < 8; b++) {
            int dx = x + b;
            if ((g[r] >> b & 1) && dx >= 0 && dx < dstw) dst[dy * dstw + dx] = col;
        }
    }
    return adv;
}

int draw_text(uint8_t *dst, int dstw, int dsth, const Font *f, int x, int y, const char *s, uint8_t col)
{
    for (; *s; s++) x += draw_char(dst, dstw, dsth, f, x, y, (uint8_t)*s, col);
    return x;
}

int text_width(const Font *f, const char *s)
{
    int w = 0;
    for (; *s; s++) w += font_char_width(f, (uint8_t)*s);
    return w;
}

/* FUN_1008_066e(step, target, start) - slide the status bar: each iteration sets the split line (and waits a vsync) */
void video_split_slide(int step, int target, int start)
{
    int v = start;
    for (;;) {
        split_line = v;
        video_wait_vsync();
        v += step;
        if (step < 0 && v < target) break;
        if (step > 0 && v > target) break;
        if (step == 0) break;
    }
    split_line = target;
}

void video_black(void) { memset(&vpal, 0, sizeof vpal); }

void video_fade_in(const Palette *target, int speed)
{
    Palette cur;
    memset(&cur, 0, sizeof cur);
    int cnt = 1, changed;
    video_set_palette(&cur);
    do {
        changed = 0;
        for (int i = 0; i < 256; i++)
            for (int k = 0; k < 3; k++)
                if (cur.c[i][k] < target->c[i][k]) { cur.c[i][k]++; changed = 1; }
        if (changed) {
            if (cnt == speed) { video_set_palette(&cur); video_wait_vsync(); cnt = 1; }
            else cnt++;
        }
    } while (changed && !quit_requested);
    video_set_palette(target);
    video_wait_vsync();
}

void video_fade_out(int speed)
{
    Palette cur = vpal;
    int cnt = 1, changed;
    do {
        changed = 0;
        for (int i = 0; i < 256; i++)
            for (int k = 0; k < 3; k++)
                if (cur.c[i][k]) { cur.c[i][k]--; changed = 1; }
        if (changed) {
            if (cnt == speed) { video_set_palette(&cur); video_wait_vsync(); cnt = 1; }
            else cnt++;
        }
    } while (changed && !quit_requested);
    video_set_palette(&cur);
    video_wait_vsync();
}
