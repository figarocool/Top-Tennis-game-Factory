/* Development tool: browse the assets of TENNIS.DAT.
 *   assetview <dir with TENNIS.DAT>   keys: Left/Right next/prev asset, Up/Down palette, Esc quit */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pak.h"
#include "gfx.h"

static Palette pal;
static int has_pal;

static const char *ext(const char *n) { const char *e = strrchr(n, '.'); return e ? e + 1 : ""; }

int main(int argc, char **argv)
{
    char path[512];
    snprintf(path, sizeof path, "%s/TENNIS.DAT", argc > 1 ? argv[1] : ".");
    if (pak_open(path)) { fprintf(stderr, "cannot open %s\n", path); return 1; }
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("assetview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 600, 0);
    SDL_Renderer *r = SDL_CreateRenderer(win, -1, 0);
    int idx = 0, running = 1;
    int palidx = -1;
    while (running) {
        const char *name = pak_name(idx);
        char title[128];
        snprintf(title, sizeof title, "%d/%d %s", idx, pak_count(), name);
        SDL_SetWindowTitle(win, title);
        SDL_SetRenderDrawColor(r, 40, 40, 40, 255);
        SDL_RenderClear(r);
        Image *im = NULL;
        if (!strcasecmp(ext(name), "PBM")) im = img_load_pbm(name);
        else if (!strcasecmp(ext(name), "CBE")) im = img_load_cbe(name);
        if (!has_pal) {
            for (int i = 0; i < 256; i++) pal.c[i][0] = pal.c[i][1] = pal.c[i][2] = i >> 2;
        }
        if (im) {
            SDL_Surface *s = SDL_CreateRGBSurface(0, im->w, im->h, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
            uint32_t *o = s->pixels;
            for (int i = 0; i < im->w * im->h; i++) {
                const uint8_t *c = pal.c[im->px[i]];
                uint32_t a = (!im->mask || im->mask[i]) ? 0xff : 0;
                o[i] = a << 24 | (c[0] * 255 / 63) << 16 | (c[1] * 255 / 63) << 8 | (c[2] * 255 / 63);
            }
            SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
            int sc = im->w <= 100 ? 6 : 2;
            SDL_Rect dst = { 10, 10, im->w * sc, im->h * sc };
            SDL_RenderCopy(r, t, NULL, &dst);
            SDL_DestroyTexture(t); SDL_FreeSurface(s);
            img_free(im);
        }
        SDL_RenderPresent(r);
        SDL_Event e;
        SDL_WaitEvent(&e);
        if (e.type == SDL_QUIT) running = 0;
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_ESCAPE: running = 0; break;
            case SDLK_RIGHT: idx = (idx + 1) % pak_count(); break;
            case SDLK_LEFT:  idx = (idx + pak_count() - 1) % pak_count(); break;
            case SDLK_PAGEUP:   idx = (idx + 20) % pak_count(); break;
            case SDLK_PAGEDOWN: idx = (idx + pak_count() - 20) % pak_count(); break;
            case SDLK_UP: case SDLK_DOWN: {
                /* cycle through all .PAL files */
                int n = pak_count(), step = e.key.keysym.sym == SDLK_UP ? 1 : n - 1;
                for (int k = 1; k <= n; k++) {
                    int j = (palidx + k * step + n) % n;
                    if (!strcasecmp(ext(pak_name(j)), "PAL")) { palidx = j; pal_load(pak_name(j), &pal); has_pal = 1; break; }
                }
                break; }
            }
        }
    }
    SDL_Quit();
    return 0;
}
