/* Joystick through SDL. The original reads the analog game port (201h) and needs a calibration; SDL gives
 * calibrated axes, so the calibration screens only walk through the same steps (centre, up-left, down-right)
 * and wait for a button, like the original does. */
#include "joy.h"
#include "matchflow.h"
#include "platform.h"
#include "video.h"
#include "text.h"
#include "dsimg.h"
#include "gfx.h"
#include "ui.h"
#include "options.h"
#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEAD 12000

static Joystick joys[2];

/* test hook: TT_VJOY="frame:x:y:buttons,..." creates a virtual joystick and drives it from the frame counter */
static SDL_JoystickID vjoy_id = -1;

void joy_script_tick(int frame)
{
    const char *e = getenv("TT_VJOY");
    if (!e) return;
    if (vjoy_id < 0) {
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
        vjoy_id = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 2, 2, 0);
        if (vjoy_id < 0) return;
    }
    SDL_Joystick *d = SDL_JoystickFromInstanceID(vjoy_id);
    if (!d) d = SDL_JoystickOpen(0);
    for (const char *q = e; q && *q; q = strchr(q, ',') ? strchr(q, ',') + 1 : NULL) {
        int f, x, y, b;
        if (sscanf(q, "%d:%d:%d:%d", &f, &x, &y, &b) == 4 && f == frame) {
            SDL_JoystickSetVirtualAxis(d, 0, (Sint16)x);
            SDL_JoystickSetVirtualAxis(d, 1, (Sint16)y);
            SDL_JoystickSetVirtualButton(d, 0, b & 1);
            SDL_JoystickSetVirtualButton(d, 1, b >> 1 & 1);
        }
    }
}

int joy_present(int n)
{
    if (!SDL_WasInit(SDL_INIT_JOYSTICK)) SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    return SDL_NumJoysticks() >= n;
}

Joystick *joy_open(int n)
{
    Joystick *j = &joys[n - 1];
    if (j->dev) return j;
    if (!joy_present(n)) return NULL;
    j->index = n - 1;
    j->dev = SDL_JoystickOpen(n - 1);
    return j->dev ? j : NULL;
}

unsigned joy_bits(Joystick *j)
{
    SDL_Joystick *d = j ? j->dev : NULL;
    if (!d) return 0;
    unsigned b = 0;
    int x = SDL_JoystickNumAxes(d) > 0 ? SDL_JoystickGetAxis(d, 0) : 0;
    int y = SDL_JoystickNumAxes(d) > 1 ? SDL_JoystickGetAxis(d, 1) : 0;
    if (SDL_JoystickNumHats(d) > 0) {
        Uint8 h = SDL_JoystickGetHat(d, 0);
        if (h & SDL_HAT_LEFT) x = -32768;
        if (h & SDL_HAT_RIGHT) x = 32767;
        if (h & SDL_HAT_UP) y = -32768;
        if (h & SDL_HAT_DOWN) y = 32767;
    }
    if (x < -DEAD) b |= 1;
    if (x > DEAD) b |= 2;
    if (y < -DEAD) b |= 4;
    if (y > DEAD) b |= 8;
    if (SDL_JoystickGetButton(d, 0)) b |= 0x10;
    if (SDL_JoystickGetButton(d, 1) || SDL_JoystickGetButton(d, 2)) b |= 0x20;
    return b;
}

static int any_button(Joystick *j) { return (joy_bits(j) & 0x30) != 0; }

/* one calibration step: show the picture and message, wait for a button press and release; ESC aborts */
static int step(Joystick *j, const char *pbm, const char *msg)
{
    Palette pal;
    memset(vpage, 0, sizeof vpage);
    Image *im = img_load_pbm(pbm);
    if (im) { blit(vpage, VW, VH, im, 0, 0); img_free(im); }
    pal_load("DATA\\PAL\\JOYSTICK.PAL", &pal);
    font_select(5);
    text_outlined("ESC exit", 0, 10, DST_PAGE, 0xb4, 0xf0);
    text_outlined(msg, 0, 14, DST_PAGE, 0x14, 0x3c);
    text_outlined("AND PRESS ANY BUTTON ...", 0, 14, DST_PAGE, 0x23, 0x3c);
    video_set_scroll(0, 0);
    video_set_palette(&pal);
    video_present();
    key_flush();
    for (;;) {
        video_wait_vsync();
        if (key_pressed_scancode() == 1 || quit_requested) return 0;
        if (any_button(j)) break;
    }
    for (;;) {
        video_wait_vsync();
        if (key_pressed_scancode() == 1 || quit_requested) return 0;
        if (!any_button(j)) break;
    }
    return 1;
}

static int calibrate(int n)
{
    Joystick *j = joy_open(n);
    if (!j) {
        message_box("          SORRY...", n == 1 ? "JOYSTICK 1 NOT DETECTED!" : "JOYSTICK 2 NOT DETECTED!", NULL);
        return MR_STAY;
    }
    video_fade_out(2);
    if (step(j, "DATA\\BACKGND\\JOY_CN.PBM", "CENTER THE JOYSTICK") &&
        step(j, "DATA\\BACKGND\\JOY_UL.PBM", "MOVE THE JOYSTICK UP-LEFT"))
        step(j, "DATA\\BACKGND\\JOY_DR.PBM", "MOVE THE JOYSTICK DOWN-RIGHT");
    video_fade_out(2);
    ui_dirty = 1;
    return MR_STAY;
}

int act_calibrate_j1(Menu *m, MenuItem *it) { (void)m; (void)it; return calibrate(1); }
int act_calibrate_j2(Menu *m, MenuItem *it) { (void)m; (void)it; return calibrate(2); }
