/* Virtual VGA screen. The original runs in an unchained 320x200 "mode X" with a 416-pixel wide virtual
 * screen, two 300-line pages (double buffering), 25 extra lines used as a status bar and a split screen.
 * Here the planar memory is replaced by chunky buffers with identical coordinates; the 70 Hz refresh is
 * emulated by video_wait_vsync(). */
#ifndef VIDEO_H
#define VIDEO_H
#include <stdint.h>
#include "gfx.h"

#define SCR_W   320
#define SCR_H   200
#define VW      416          /* virtual width  (stride 104 bytes * 4 planes) */
#define VH      300          /* page height */
#define HUD_H   25           /* status bar lines shown below the split line */

extern uint8_t vpage[VW * VH];       /* page being drawn / displayed */
extern uint8_t vhud[VW * HUD_H];     /* status bar */
extern Palette vpal;                 /* current DAC contents */
extern int     scroll_x, scroll_y;   /* display start (pixels) */
extern int     split_line;           /* first scanline of the status bar; 200 = no split */

int  video_init(void);
void video_shutdown(void);
void video_toggle_fullscreen(void);
void video_refresh(void);                /* re-display the last composed frame */
void video_present(void);                /* compose current state and show it */
void video_wait_vsync(void);             /* present + wait for next 70 Hz tick (FUN_1018_1bd1) */
void video_set_palette(const Palette *p);
void video_set_scroll(int y, int x);     /* FUN_1018_1aca / 1792 */
void video_set_split(int line);
void video_fade_in(const Palette *target, int speed);   /* FUN_1010_39cc: from black, 1 DAC step per cycle, shown every `speed` cycles */
void video_fade_out(int speed);                          /* FUN_1010_38d3 */
void video_black(void);                                  /* FUN_1010_38a4: set an all-black palette */
void video_split_slide(int step, int target, int start);   /* FUN_1008_066e: move the split line one step per vsync */

/* drawing helpers; dst is VW wide */
void blit(uint8_t *dst, int dstw, int dsth, const Image *src, int x, int y);         /* opaque if no mask, else transparent */
void fill_rect(uint8_t *dst, int dstw, int dsth, int x, int y, int w, int h, uint8_t c);
void clear_page(uint8_t c);

/* text with a .FNT font (bit0 = leftmost); returns pixel advance */
int  draw_char(uint8_t *dst, int dstw, int dsth, const Font *f, int x, int y, int ch, uint8_t col);
int  draw_text(uint8_t *dst, int dstw, int dsth, const Font *f, int x, int y, const char *s, uint8_t col);
int  text_width(const Font *f, const char *s);
#endif
