/* Exercise the console input path with an SDL virtual controller, without game data. */
#include "../src/platform.h"
#include <SDL.h>
#include <assert.h>
#include <stdio.h>

int g_aspect;
void video_toggle_fullscreen(void) {}
void video_set_aspect(int mode) { g_aspect = mode; }
void video_refresh(void) {}
void video_touch_to_game(float nx, float ny, float *x, float *y)
{
    *x = nx * 320; *y = ny * 200;
}
void joy_script_tick(int frame) { (void)frame; }

static void poll_many(int count)
{
    while (count--) { SDL_JoystickUpdate(); plat_poll(); }
}

int main(void)
{
    assert(plat_init() == 0);
    int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 2, 12, 0);
    assert(device >= 0);
    SDL_Joystick *pad = SDL_JoystickOpen(device);
    assert(pad);
    assert(plat_pointer_mode(1) == 0);
    assert(SDL_JoystickSetVirtualAxis(pad, 0, 20000) == 0);
    poll_many(1);
    assert(SDL_JoystickSetVirtualAxis(pad, 0, 0) == 0);
    poll_many(1);
    int x, y, parked_x, parked_y;
    assert(plat_mouse(&parked_x, &parked_y) == 0);
    /* Frame pacing pumps events several times before the next pointer read. */
    poll_many(1000);
    for (int letter = 0; letter < 3; letter++) {
        assert(SDL_JoystickSetVirtualButton(pad, 2, 1) == 0);
        poll_many(1000);
        assert(plat_mouse(&x, &y) == 1);
        assert(x == parked_x && y == parked_y);
        assert(!keys[0x1c] && !keys[0x39]); /* A click must not become Enter/Space. */
        assert(SDL_JoystickSetVirtualButton(pad, 2, 0) == 0);
        poll_many(1000);
        assert(plat_mouse(&x, &y) == 0);
    }
    int nested = plat_pointer_mode(1);
    assert(nested == 1);
    plat_pointer_mode(nested);
    /* D-pad explicitly switches back to keyboard navigation. */
    assert(SDL_JoystickSetVirtualButton(pad, 8, 1) == 0);
    poll_many(1);
    assert(keys[0xc8]);
    assert(SDL_JoystickSetVirtualButton(pad, 2, 1) == 0);
    poll_many(1);
    assert(plat_mouse(&x, &y) == 0 && keys[0x1c] && keys[0x39]);
    assert(SDL_JoystickSetVirtualButton(pad, 8, 0) == 0);
    assert(SDL_JoystickSetVirtualButton(pad, 2, 0) == 0);
    poll_many(1);
    /* Returning to play restores analog movement and the fire button. */
    assert(plat_pointer_mode(0) == 1);
    assert(SDL_JoystickSetVirtualAxis(pad, 0, 20000) == 0);
    assert(SDL_JoystickSetVirtualButton(pad, 2, 1) == 0);
    poll_many(1);
    assert(keys[0xcd] && keys[0x1c] && keys[0x39]);
    SDL_JoystickClose(pad);
    assert(SDL_JoystickDetachVirtual(device) == 0);
    plat_quit();
    puts("console pointer: repeated stationary clicks, polling, navigation and gameplay: OK");
    return 0;
}
