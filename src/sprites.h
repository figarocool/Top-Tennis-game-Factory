/* Sprite manager (original unit at 1008:2b06..30d5).
 * 800 sprite definitions (compiled sprites from TENNIS.DAT) and 100 sprite instances ("handles").
 * An instance is anchored at its bottom-left corner: it is drawn with its top-left at (x, y - h).
 * Instances are drawn in ascending handle order, hidden ones skipped. */
#ifndef SPRITES_H
#define SPRITES_H
#include <stdint.h>
#include "gfx.h"

#define MAX_DEFS    800
#define MAX_HANDLES 100

typedef struct {
    int exists;
    int id;           /* current definition */
    int x, y;         /* bottom-left anchor */
    int w, h;
    int hidden;
} Sprite;

extern Sprite spr[MAX_HANDLES];
extern int    spr_max_handle;
extern int    spr_order[MAX_HANDLES];   /* draw order (position -> handle), DS:BE84 */

void spr_init(void);                               /* FUN_1008_2b06 */
int  spr_load_def(const char *name, int id);       /* FUN_1008_359a (compiled sprite) */
void spr_free_def(int id);
void spr_create(int handle, int x, int y);         /* FUN_1008_2b98 (buffer size argument is irrelevant here) */
void spr_destroy(int handle);
void spr_set_def(int handle, int id);              /* FUN_1008_2d62 */
void spr_hide(int handle);                         /* FUN_1008_2dd8 */
void spr_show(int handle);                         /* FUN_1008_2e0c */
void spr_pos(int handle, int y, int x);            /* FUN_1008_2f96 (note: y first, like the original) */
void spr_move_clamped(int handle, int ymax, int xmax, int ymin, int xmin, int dy, int dx);  /* FUN_1008_2fe1 */
Sprite *spr_get(int handle);                       /* FUN_1008_309f */

void spr_sort(int first, int last, int ball_handle, int shadow_handle);   /* FUN_1000_666b: depth sort by y */
void spr_set_background(const Image *bg);          /* clean background restored before every redraw */
void spr_clear_background(void);
void spr_render(int scroll_y, int scroll_x);       /* FUN_1008_30d5: redraw all + set scroll */
#endif
