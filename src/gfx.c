#include "gfx.h"
#include "pak.h"
#include <stdlib.h>
#include <string.h>

#define VSTRIDE 104        /* bytes per row of the virtual screen the compiled sprites were built for */

Image *img_new(int w, int h, int masked)
{
    Image *im = calloc(1, sizeof *im);
    im->w = w; im->h = h;
    im->px = calloc((size_t)w * h, 1);
    if (masked) im->mask = calloc((size_t)w * h, 1);
    return im;
}

void img_free(Image *im)
{
    if (!im) return;
    free(im->px); free(im->mask); free(im);
}

/* PBM: u8 width/4, u8 height, then 4 planes of (width/4 * height) bytes. */
Image *img_load_pbm(const char *name)
{
    size_t n;
    uint8_t *d = pak_load(name, &n);
    if (!d || n < 2) { free(d); return NULL; }
    int wb = d[0], h = d[1], w = wb * 4;
    size_t plane = (size_t)wb * h;
    if (n < 2 + plane * 4) { free(d); return NULL; }
    Image *im = img_new(w, h, 0);
    for (int p = 0; p < 4; p++)
        for (size_t i = 0; i < plane; i++) {
            int row = i / wb, col = i % wb;
            im->px[row * w + col * 4 + p] = d[2 + p * plane + i];
        }
    free(d);
    return im;
}

int pal_load(const char *name, Palette *p)
{
    size_t n;
    uint8_t *d = pak_load(name, &n);
    if (!d || n < 768) { free(d); return -1; }
    memcpy(p->c, d, 768);
    free(d);
    return 0;
}

/* CBE: 2 header bytes (width in bytes, height), then x86 code:
 *   C6 /0 modrm disp imm8     mov byte [si+disp], imm8
 *   C7 /0 modrm disp imm16    mov word [si+disp], imm16
 *   D0 C0 83 D6 00 EE         next plane (rol al,1 ; adc si,0 ; out dx,al)
 *   CB                        retf
 * The caller biases SI by +0x80 so the real offset is disp+128. */
Image *img_load_cbe(const char *name)
{
    size_t n;
    uint8_t *d = pak_load(name, &n);
    if (!d || n < 3) { free(d); return NULL; }
    int w = d[0] * 4, h = d[1];
    Image *im = img_new(w, h, 1);
    int plane = 0;
    size_t i = 2;
    while (i < n) {
        uint8_t op = d[i];
        if (op == 0xC6 || op == 0xC7) {
            int disp;
            if (i + 2 > n) break;
            uint8_t m = d[i + 1];
            if (m == 0x44)      { if (i + 3 > n) break; disp = (int8_t)d[i + 2]; i += 3; }
            else if (m == 0x84) { if (i + 4 > n) break; disp = (int16_t)(d[i + 2] | d[i + 3] << 8); i += 4; }
            else if (m == 0x04) { disp = 0; i += 2; }
            else break;
            int nbytes = op == 0xC6 ? 1 : 2;
            for (int k = 0; k < nbytes && i < n; k++, i++) {
                int off = disp + 128 + k;
                int row = off / VSTRIDE, col = off % VSTRIDE;
                int x = col * 4 + plane;
                if (x >= 0 && x < w && row >= 0 && row < h) {
                    im->px[row * w + x] = d[i];
                    im->mask[row * w + x] = 1;
                }
            }
        } else if (op == 0xD0 && i + 6 <= n && d[i + 1] == 0xC0 && d[i + 2] == 0x83 &&
                   d[i + 3] == 0xD6 && d[i + 4] == 0x00 && d[i + 5] == 0xEE) {
            plane++; i += 6;
        } else {
            break;                        /* CB (retf) or end */
        }
    }
    free(d);
    return im;
}

Font *font_load(const char *name)
{
    size_t n;
    uint8_t *d = pak_load(name, &n);
    if (!d || n < 4) { free(d); return NULL; }
    Font *f = calloc(1, sizeof *f);
    f->first   = d[0];
    f->height  = d[2];
    f->advance = d[3];
    int rec = f->height + (f->advance == 0);
    if (!rec) { free(d); free(f); return NULL; }
    f->count = (int)((n - 4) / rec);
    f->glyphs = malloc(n - 4);
    memcpy(f->glyphs, d + 4, n - 4);
    free(d);
    return f;
}

void font_free(Font *f) { if (f) { free(f->glyphs); free(f); } }

int font_char_width(const Font *f, int ch)
{
    if (f->advance) return f->advance;
    int i = ch - f->first;
    if (i < 0 || i >= f->count) return 0;
    return f->glyphs[i * (f->height + 1) + f->height];
}
