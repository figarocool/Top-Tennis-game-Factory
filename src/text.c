#include "text.h"
#include "video.h"
#include <stdlib.h>

static Font *slots[FONT_SLOTS];
static int cur = -1;

int font_slot_load(const char *name, int slot)
{
    if ((unsigned)slot >= FONT_SLOTS) return 0;
    Font *f = font_load(name);
    if (!f) return 0;
    font_free(slots[slot]);
    slots[slot] = f;
    return 1;
}

void font_select(int slot) { if ((unsigned)slot < FONT_SLOTS && slots[slot]) cur = slot; }
int  font_slot_height(int slot) { return ((unsigned)slot < FONT_SLOTS && slots[slot]) ? slots[slot]->height : 0; }
const Font *font_current(void) { return cur >= 0 ? slots[cur] : NULL; }

static uint8_t *dst_buf(int dst, int *w, int *h)
{
    if (dst == DST_HUD) { *w = VW; *h = HUD_H; return vhud; }
    *w = VW; *h = VH; return vpage;
}

int text_at(const char *s, int color, int dst, int y, int x)
{
    const Font *f = font_current();
    if (!f) return x;
    int w, h;
    uint8_t *d = dst_buf(dst, &w, &h);
    return draw_text(d, w, h, f, x, y, s, (uint8_t)color);
}

void text_outlined(const char *s, int outline, int fill, int dst, int y, int x)
{
    static const int off[8][2] = { {-1,0}, {-1,1}, {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1} };  /* (dy,dx) as 055e */
    for (int i = 0; i < 8; i++) text_at(s, outline, dst, y + off[i][0], x + off[i][1]);
    text_at(s, fill, dst, y, x);
}

int text_pix_width(const char *s)
{
    const Font *f = font_current();
    return f ? text_width(f, s) : 0;
}

void dst_image(int dst, const Image *im, int y, int x)
{
    int w, h;
    uint8_t *d = dst_buf(dst, &w, &h);
    blit(d, w, h, im, x, y);
}

void text_glyph(int ch, int color, int y, int x)
{
    const Font *f = font_current();
    if (f) draw_char(vpage, VW, VH, f, x, y, ch, (uint8_t)color);
}
