/* SDL glue: window, event pump, DOS-style timing and keyboard state.
 * The original polls a 256-entry key-state array (DS:CC12, indexed by XT scancode) fed by an INT 9 ISR
 * and reads a free-running 1.193182 MHz PIT counter for all game timing. */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>

#define PIT_HZ 1193182u

/* keys[sc] != 0 while the key with DOS scancode sc is held down. Extended keys (arrows, ...) are
 * stored with bit 7 set (0xC8 = up, 0xD0 = down, 0xCB = left, 0xCD = right), as in TENNIS.OPT. */
extern uint8_t keys[256];

int      plat_init(void);
void     plat_quit(void);
int      plat_mouse(int *x, int *y);          /* pointer in 320x200 game pixels; returns 1 while the button is down.
                                               * PC: the mouse. Vita: left stick moves it, cross or a tap clicks */
int      plat_touch_get(float *x, float *y);   /* pop one finger-down (normalised 0..1 screen coords), 0 if none */
void     plat_poll(void);                /* pump SDL events, update keys[]; sets quit_requested on window close */
extern int quit_requested;

uint32_t timer_ticks(void);              /* PIT ticks (1193182 Hz), 32-bit wrap */
void     plat_sleep_ms(int ms);

/* keyboard buffer used by menus (FUN_1008_3d73 = ReadKey-style: returns DOS scancode of last press, 0 if none) */
int      key_pressed_scancode(void);     /* pop one scancode or 0 */
void     key_flush(void);

/* Timers 1..25 (FUN_1008_2975 start / FUN_1008_29aa elapsed) */
void     timer_start(int id);
uint32_t timer_elapsed(int id);

/* test hook: scripted key events driven by the frame counter (env TT_INPUT) */
void     plat_script_tick(int frame);

/* Turbo Pascal Random(n) */
uint16_t rnd(uint16_t n);
void     rnd_seed(uint32_t s);
#endif
