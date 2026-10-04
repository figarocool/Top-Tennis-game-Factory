#include "sprites.h"
#include "video.h"
#include <stdlib.h>
#include <string.h>

Sprite spr[MAX_HANDLES];
int    spr_max_handle;
int    spr_order[MAX_HANDLES];

static Image *defs[MAX_DEFS];
static uint8_t *bgbuf;

void spr_init(void)
{
    memset(spr, 0, sizeof spr);
    spr_max_handle = 0;
    for (int i = 0; i < MAX_HANDLES; i++) spr_order[i] = i;
}

int spr_load_def(const char *name, int id)
{
    if ((unsigned)id >= MAX_DEFS) return 0;
    Image *im = img_load_cbe(name);
    if (!im) return 0;
    img_free(defs[id]);
    defs[id] = im;
    return 1;
}

void spr_free_def(int id)
{
    if ((unsigned)id < MAX_DEFS) { img_free(defs[id]); defs[id] = NULL; }
}

void spr_create(int handle, int x, int y)
{
    if ((unsigned)handle >= MAX_HANDLES) return;
    memset(&spr[handle], 0, sizeof spr[handle]);
    spr[handle].exists = 1;
    spr[handle].x = x;
    spr[handle].y = y;
    if (handle > spr_max_handle) spr_max_handle = handle;
}

void spr_destroy(int handle)
{
    if ((unsigned)handle < MAX_HANDLES) spr[handle].exists = 0;
}

void spr_set_def(int handle, int id)
{
    if ((unsigned)handle >= MAX_HANDLES || (unsigned)id >= MAX_DEFS) return;
    if (!spr[handle].exists || !defs[id]) return;
    spr[handle].id = id;
    spr[handle].w = defs[id]->w;
    spr[handle].h = defs[id]->h;
}

void spr_hide(int handle) { if ((unsigned)handle < MAX_HANDLES && spr[handle].exists) spr[handle].hidden = 1; }
void spr_show(int handle) { if ((unsigned)handle < MAX_HANDLES && spr[handle].exists) spr[handle].hidden = 0; }

void spr_pos(int handle, int y, int x)
{
    if ((unsigned)handle < MAX_HANDLES && spr[handle].exists) { spr[handle].x = x; spr[handle].y = y; }
}

void spr_move_clamped(int handle, int ymax, int xmax, int ymin, int xmin, int dy, int dx)
{
    Sprite *s = spr_get(handle);
    if (!s) return;
    s->x += dx;
    s->y += dy;
    if (s->x < xmin) s->x = xmin; else if (xmax < s->x) s->x = xmax;
    if (ymax < s->y) s->y = ymax; else if (s->y < ymin) s->y = ymin;
}

Sprite *spr_get(int handle)
{
    return ((unsigned)handle < MAX_HANDLES && spr[handle].exists) ? &spr[handle] : NULL;
}

void spr_set_background(const Image *bg)
{
    if (!bgbuf) bgbuf = malloc(VW * VH);
    memset(bgbuf, 0, VW * VH);
    for (int y = 0; y < bg->h && y < VH; y++)
        memcpy(bgbuf + y * VW, bg->px + y * bg->w, bg->w < VW ? bg->w : VW);
}

void spr_clear_background(void) { free(bgbuf); bgbuf = NULL; }

void spr_render(int scroll_y, int scroll_x)
{
    if (bgbuf) memcpy(vpage, bgbuf, VW * VH);
    for (int i = 0; i <= spr_max_handle; i++) {
        const Sprite *s = &spr[spr_order[i]];
        if (!s->exists || s->hidden || !defs[s->id]) continue;
        blit(vpage, VW, VH, defs[s->id], s->x, s->y - s->h);
    }
    video_set_scroll(scroll_y, scroll_x);
}

/* 1000:64ca - order of two sprites. The ball is ranked by its shadow's y; at equal y the shadow goes first. */
static int sort_ball, sort_shadow;
static int cmp_spr(int a, int b)
{
    const Sprite *sh = &spr[sort_shadow], *bl = &spr[sort_ball];
    const Sprite *sa = &spr[a], *sb = &spr[b];
    const Sprite *ea = (sa == bl) ? sh : sa;
    const Sprite *eb = (sb == bl) ? sh : sb;
    if (ea->y < eb->y) return -1;
    if (eb->y < ea->y) return 1;
    if (sa == bl && sb == sh) return 1;
    if (sa == sh && sb == bl) return -1;
    return 0;
}

static void swap_order(int i, int j)
{
    /* 1000:65fe swaps only when the two sprites are at different y */
    int a = spr_order[i], b = spr_order[j];
    if (spr[a].y != spr[b].y) { spr_order[i] = b; spr_order[j] = a; }
}

/* 1008:3929 Hoare quicksort over positions l..r */
static void qsort_order(int l, int r)
{
    int i = l, j = r;
    int pivot = spr_order[(l + r) >> 1];
    do {
        while (cmp_spr(spr_order[i], pivot) < 0) i++;
        while (cmp_spr(pivot, spr_order[j]) < 0) j--;
        if (i <= j) { swap_order(i, j); i++; j--; }
    } while (i <= j);
    if (l < j) qsort_order(l, j);
    if (i < r) qsort_order(i, r);
}

void spr_sort(int first, int last, int ball_handle, int shadow_handle)
{
    sort_ball = ball_handle;
    sort_shadow = shadow_handle;
    qsort_order(first, last);
}
