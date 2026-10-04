/* Generated PBM images exercise missing halves and clipping without game assets. */
#include "../src/gamecfg.h"
#include "../src/game.h"
#include "../src/video.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t g_court_type;
static int first_present, first_w, first_h, second_w, second_h, checks;

uint8_t *pak_load(const char *name, size_t *size)
{
    int first = strstr(name, "1A.PBM") != NULL;
    assert(first || strstr(name, "1B.PBM"));
    if (first && !first_present) { *size = 0; return NULL; }
    int w = first ? first_w : second_w, h = first ? first_h : second_h;
    *size = 2 + (size_t)w * h;
    uint8_t *d = malloc(*size); assert(d);
    d[0] = w / 4; d[1] = h;
    memset(d + 2, first ? 1 : 2, *size - 2);
    return d;
}

void spr_set_background(const Image *bg)
{
    assert(bg->w == VW && bg->h == VH);
    for (int y = 0; y < VH; y++) for (int x = 0; x < VW; x++) {
        int value = first_present && y < first_h && x < first_w ? 1 : 0;
        int start = first_present ? first_h : 0;
        if (y >= start && y < start + second_h && x < second_w) value = 2;
        assert(bg->px[y * VW + x] == value);
    }
    checks++;
}

int main(void)
{
    first_present = 1; first_w = second_w = VW; first_h = second_h = VH / 2;
    court_select(1); /* Complete, correctly sized background. */
    first_present = 0;
    court_select(1); /* Missing first half must not dereference a null image. */
    first_present = 1; first_w = 8; first_h = 2; second_w = 16; second_h = 3;
    court_select(1); /* Short rows must retain their own stride. */
    first_w = second_w = 1020; first_h = second_h = 255;
    court_select(1); /* Large source images must be clipped to the destination. */
    assert(checks == 4 && g_court_type == 1);
    puts("court backgrounds: full images, missing halves, short rows and clipping: OK");
    return 0;
}
