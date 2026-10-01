#ifndef GFX_H
#define GFX_H
#include <stdint.h>

/* 8-bit chunky image. If mask != NULL it has one byte per pixel (0 = transparent). */
typedef struct {
    int      w, h;
    uint8_t *px;
    uint8_t *mask;
} Image;

typedef struct { uint8_t c[256][3]; } Palette;      /* VGA 6-bit components (0..63) */

/* Bitmap font (.FNT): bit 0 of every row byte is the leftmost pixel. */
typedef struct {
    int      first;       /* first character code */
    int      count;
    int      height;
    int      advance;     /* fixed advance; 0 = proportional (extra width byte after each glyph) */
    uint8_t *glyphs;
} Font;

Image *img_new(int w, int h, int masked);
void   img_free(Image *im);
Image *img_load_pbm(const char *name);      /* planar mode-X bitmap from TENNIS.DAT */
Image *img_load_cbe(const char *name);      /* compiled sprite decoded to a masked image */
int    pal_load(const char *name, Palette *p);
Font  *font_load(const char *name);
void   font_free(Font *f);
int    font_char_width(const Font *f, int ch);
#endif
