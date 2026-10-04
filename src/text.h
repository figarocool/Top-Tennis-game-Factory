/* Font slots and text output (units 1008:36d3/3889 and 1010:3b56, 1008:055e). */
#ifndef TEXT_H
#define TEXT_H
#include <stdint.h>
#include "gfx.h"

#define FONT_SLOTS 10
enum { DST_PAGE = 0, DST_HUD = 1 };      /* where text/images go: the 416x300 page or the status bar */

int  font_slot_load(const char *name, int slot);          /* FUN_1000_e6e5 */
void font_select(int slot);                                /* FUN_1008_3889 */
int  font_slot_height(int slot);                           /* FUN_1008_38d0 */
const Font *font_current(void);

int  text_at(const char *s, int color, int dst, int y, int x);                     /* FUN_1010_3b56, returns end x */
void text_outlined(const char *s, int outline, int fill, int dst, int y, int x);   /* FUN_1008_055e */
void text_glyph(int ch, int color, int y, int x);                                      /* FUN_1018_29d1: a single character */
int  text_pix_width(const char *s);
void dst_image(int dst, const Image *im, int y, int x);                            /* FUN_1018_0601 / 058d */
#endif
