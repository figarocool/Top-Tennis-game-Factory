#include "platform.h"
#include "joy.h"
#include <SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "video.h"

uint8_t keys[256];
int     quit_requested;

static uint8_t kbuf[64];
static int     kh, kt;

static void push_key(int sc)
{
    int nt = (kt + 1) % (int)sizeof kbuf;
    if (nt != kh) { kbuf[kt] = (uint8_t)sc; kt = nt; }
}

/* SDL scancode -> DOS (XT set 1) make code */
static uint8_t sc_map[SDL_NUM_SCANCODES];

static void build_map(void)
{
    static const struct { int sdl; uint8_t dos; } m[] = {
        { SDL_SCANCODE_ESCAPE, 0x01 }, { SDL_SCANCODE_1, 0x02 }, { SDL_SCANCODE_2, 0x03 }, { SDL_SCANCODE_3, 0x04 },
        { SDL_SCANCODE_4, 0x05 }, { SDL_SCANCODE_5, 0x06 }, { SDL_SCANCODE_6, 0x07 }, { SDL_SCANCODE_7, 0x08 },
        { SDL_SCANCODE_8, 0x09 }, { SDL_SCANCODE_9, 0x0a }, { SDL_SCANCODE_0, 0x0b }, { SDL_SCANCODE_MINUS, 0x0c },
        { SDL_SCANCODE_EQUALS, 0x0d }, { SDL_SCANCODE_BACKSPACE, 0x0e }, { SDL_SCANCODE_TAB, 0x0f },
        { SDL_SCANCODE_Q, 0x10 }, { SDL_SCANCODE_W, 0x11 }, { SDL_SCANCODE_E, 0x12 }, { SDL_SCANCODE_R, 0x13 },
        { SDL_SCANCODE_T, 0x14 }, { SDL_SCANCODE_Y, 0x15 }, { SDL_SCANCODE_U, 0x16 }, { SDL_SCANCODE_I, 0x17 },
        { SDL_SCANCODE_O, 0x18 }, { SDL_SCANCODE_P, 0x19 }, { SDL_SCANCODE_LEFTBRACKET, 0x1a },
        { SDL_SCANCODE_RIGHTBRACKET, 0x1b }, { SDL_SCANCODE_RETURN, 0x1c }, { SDL_SCANCODE_LCTRL, 0x1d },
        { SDL_SCANCODE_A, 0x1e }, { SDL_SCANCODE_S, 0x1f }, { SDL_SCANCODE_D, 0x20 }, { SDL_SCANCODE_F, 0x21 },
        { SDL_SCANCODE_G, 0x22 }, { SDL_SCANCODE_H, 0x23 }, { SDL_SCANCODE_J, 0x24 }, { SDL_SCANCODE_K, 0x25 },
        { SDL_SCANCODE_L, 0x26 }, { SDL_SCANCODE_SEMICOLON, 0x27 }, { SDL_SCANCODE_APOSTROPHE, 0x28 },
        { SDL_SCANCODE_GRAVE, 0x29 }, { SDL_SCANCODE_LSHIFT, 0x2a }, { SDL_SCANCODE_BACKSLASH, 0x2b },
        { SDL_SCANCODE_Z, 0x2c }, { SDL_SCANCODE_X, 0x2d }, { SDL_SCANCODE_C, 0x2e }, { SDL_SCANCODE_V, 0x2f },
        { SDL_SCANCODE_B, 0x30 }, { SDL_SCANCODE_N, 0x31 }, { SDL_SCANCODE_M, 0x32 }, { SDL_SCANCODE_COMMA, 0x33 },
        { SDL_SCANCODE_PERIOD, 0x34 }, { SDL_SCANCODE_SLASH, 0x35 }, { SDL_SCANCODE_RSHIFT, 0x36 },
        { SDL_SCANCODE_LALT, 0x38 }, { SDL_SCANCODE_SPACE, 0x39 }, { SDL_SCANCODE_CAPSLOCK, 0x3a },
        { SDL_SCANCODE_F1, 0x3b }, { SDL_SCANCODE_F2, 0x3c }, { SDL_SCANCODE_F3, 0x3d }, { SDL_SCANCODE_F4, 0x3e },
        { SDL_SCANCODE_F5, 0x3f }, { SDL_SCANCODE_F6, 0x40 }, { SDL_SCANCODE_F7, 0x41 }, { SDL_SCANCODE_F8, 0x42 },
        { SDL_SCANCODE_F9, 0x43 }, { SDL_SCANCODE_F10, 0x44 },
        { SDL_SCANCODE_KP_7, 0x47 }, { SDL_SCANCODE_KP_8, 0x48 }, { SDL_SCANCODE_KP_9, 0x49 },
        { SDL_SCANCODE_KP_4, 0x4b }, { SDL_SCANCODE_KP_5, 0x4c }, { SDL_SCANCODE_KP_6, 0x4d },
        { SDL_SCANCODE_KP_1, 0x4f }, { SDL_SCANCODE_KP_2, 0x50 }, { SDL_SCANCODE_KP_3, 0x51 },
        { SDL_SCANCODE_KP_0, 0x52 }, { SDL_SCANCODE_KP_PERIOD, 0x53 },
        { SDL_SCANCODE_HOME, 0xc7 }, { SDL_SCANCODE_UP, 0xc8 }, { SDL_SCANCODE_PAGEUP, 0xc9 },
        { SDL_SCANCODE_LEFT, 0xcb }, { SDL_SCANCODE_RIGHT, 0xcd }, { SDL_SCANCODE_END, 0xcf },
        { SDL_SCANCODE_DOWN, 0xd0 }, { SDL_SCANCODE_PAGEDOWN, 0xd1 }, { SDL_SCANCODE_INSERT, 0xd2 },
        { SDL_SCANCODE_DELETE, 0xd3 }, { SDL_SCANCODE_KP_ENTER, 0x9c }, { SDL_SCANCODE_RCTRL, 0x9d },
        { SDL_SCANCODE_RALT, 0xb8 },
    };
    for (unsigned i = 0; i < sizeof m / sizeof m[0]; i++) sc_map[m[i].sdl] = m[i].dos;
}

int plat_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) < 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return -1;
    }
    build_map();
    return 0;
}

void plat_quit(void) { SDL_Quit(); }

#if defined(__vita__) || defined(__PSP__)
/* The Vita and the PSP have no keyboard: the pad drives the same key table the DOS game reads.
 * d-pad / left stick = arrows, cross = Enter+Space (fire), circle = Esc, triangle = F3 (replay), square = S,
 * start = F5 (pause), select = F10, L = Y, R = N. */
static float vcur_x = 160, vcur_y = 100;
static int   vcur_btn, vmenu_mode, vcur_active;
static Uint32 vtap_until;

static void vita_pad(void)
{
    static SDL_Joystick *pad;
    static uint8_t held[256];
    if (!pad) {
        static int tries;
        if (SDL_NumJoysticks() < 1) { if (tries++ == 0) fprintf(stderr, "pad: no joystick found\n"); return; }
        pad = SDL_JoystickOpen(0);
        if (!pad) { fprintf(stderr, "pad: cannot open the joystick: %s\n", SDL_GetError()); return; }
        fprintf(stderr, "pad: %s, %d buttons, %d axes\n", SDL_JoystickName(pad), SDL_JoystickNumButtons(pad), SDL_JoystickNumAxes(pad));
    }
    uint8_t want[256];
    memset(want, 0, sizeof want);
    int ax = SDL_JoystickGetAxis(pad, 0), ay = SDL_JoystickGetAxis(pad, 1);
    #define B(n) SDL_JoystickGetButton(pad, n)
    int menu = vmenu_mode > 0;                       /* a menu or dialog is polling the pointer */
    if (vmenu_mode > 0) vmenu_mode--;
    want[0xc8] = B(8) || (!menu && ay < -16000);  want[0xd0] = B(6) || (!menu && ay > 16000);
    want[0xcb] = B(7) || (!menu && ax < -16000);  want[0xcd] = B(9) || (!menu && ax > 16000);
    if (menu) {                                      /* left stick = mouse pointer, cross = click while it is in use */
        int dx = abs(ax) > 6000 ? ax : 0, dy = abs(ay) > 6000 ? ay : 0;
        if (dx || dy) {
            vcur_x += dx / 32768.0f * 3.0f; vcur_y += dy / 32768.0f * 3.0f;
            vcur_x = vcur_x < 0 ? 0 : vcur_x > 319 ? 319 : vcur_x;
            vcur_y = vcur_y < 0 ? 0 : vcur_y > 199 ? 199 : vcur_y;
            vcur_active = 200;
        } else if (vcur_active > 0) vcur_active--;
        if (B(6) || B(7) || B(8) || B(9)) vcur_active = 0;
    } else vcur_active = 0;
    vcur_btn = vcur_active > 0 && B(2);
    int pointer_click = vcur_active > 0 && menu;
    want[0x1c] = want[0x39] = B(2) && !pointer_click;  want[0x01] = B(1);
    want[0x3d] = B(0);                 want[0x1f] = B(3);
    want[0x3f] = B(11);                want[0x44] = B(10);
    want[0x15] = B(4);                 want[0x31] = B(5);
    #undef B
    for (int i = 1; i < 256; i++) {
        if (want[i] == held[i]) continue;
        held[i] = want[i];
        { static int logged; if (logged++ < 40) fprintf(stderr, "pad: key %02x %s\n", i, want[i] ? "down" : "up"); }
        if (want[i] && !keys[i]) push_key(i);
        keys[i] = want[i];
    }
}
#endif

static float touch_x, touch_y;
static int   touch_pending;

int plat_touch_get(float *x, float *y)
{
    if (!touch_pending) return 0;
    touch_pending = 0;
    *x = touch_x; *y = touch_y;
    return 1;
}

void plat_poll(void)
{
#if defined(__vita__) || defined(__PSP__)
    vita_pad();
#endif
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT: quit_requested = 1; break;
        case SDL_FINGERDOWN: touch_x = e.tfinger.x; touch_y = e.tfinger.y; touch_pending = 1;
#ifdef __vita__
            {
                video_touch_to_game(e.tfinger.x, e.tfinger.y, &vcur_x, &vcur_y);
                vcur_x = vcur_x < 0 ? 0 : vcur_x > 319 ? 319 : vcur_x;
                vcur_y = vcur_y < 0 ? 0 : vcur_y > 199 ? 199 : vcur_y;
                vtap_until = SDL_GetTicks() + 70;
            }
#endif
            break;
        case SDL_KEYDOWN: case SDL_KEYUP: {
            int sc = sc_map[e.key.keysym.scancode];
            if (!sc) break;
            int down = e.type == SDL_KEYDOWN;
            if (down && !e.key.repeat && !keys[sc]) push_key(sc);
            keys[sc] = down;
            if (down && e.key.keysym.scancode == SDL_SCANCODE_F11) video_toggle_fullscreen();
            if (down && e.key.keysym.scancode == SDL_SCANCODE_F12) video_set_aspect(g_aspect == 2 ? 1 : 2);
            break; }
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_EXPOSED) video_refresh();
            break;
        }
    }
}

int key_pressed_scancode(void)
{
    plat_poll();
    if (kh == kt) return 0;
    int v = kbuf[kh];
    kh = (kh + 1) % (int)sizeof kbuf;
    return v;
}

void key_flush(void) { plat_poll(); kh = kt = 0; }

uint32_t timer_ticks(void)
{
    static uint64_t base;
    uint64_t f = SDL_GetPerformanceFrequency(), c = SDL_GetPerformanceCounter();
    if (!base) base = c;
    static int turbo = -1;                          /* test hook: TT_TURBO=n runs the game's clock n times faster */
    if (turbo < 0) turbo = getenv("TT_TURBO") ? atoi(getenv("TT_TURBO")) : 1;
    return (uint32_t)(((c - base) * PIT_HZ * (turbo > 0 ? turbo : 1)) / f);
}

void plat_sleep_ms(int ms) { SDL_Delay(ms); }

#define NTIMERS 26
static uint32_t tstart[NTIMERS];
void timer_start(int id) { if (id > 0 && id < NTIMERS) tstart[id] = timer_ticks(); }
uint32_t timer_elapsed(int id)
{
    if (id <= 0 || id >= NTIMERS) return 0;
    return timer_ticks() - tstart[id];
}

void plat_script_tick(int frame)
{
    joy_script_tick(frame);
    const char *sc = getenv("TT_INPUT");
    if (!sc) return;
    for (const char *q = sc; *q;) {
        int f, k, d;
        if (sscanf(q, "%d:%d:%d", &f, &k, &d) == 3 && f == frame && k >= 0 && k < 256) { if (d && !keys[k]) push_key(k); keys[k] = d; }
        while (*q && *q != ',') q++;
        if (*q == ',') q++;
    }
}

static uint32_t seed = 1;
void rnd_seed(uint32_t s) { seed = s; }
uint16_t rnd(uint16_t n)
{
    seed = seed * 134775813u + 1;                    /* BP7 System.Random */
    return (uint16_t)(((uint64_t)(seed >> 16) * n) >> 16);
}

int plat_mouse(int *x, int *y)
{
#if defined(__vita__) || defined(__PSP__)
    vmenu_mode = 4;
    *x = (int)vcur_x; *y = (int)vcur_y;
    return vcur_btn || SDL_GetTicks() < vtap_until;
#else
    int mx, my;
    Uint32 b = SDL_GetMouseState(&mx, &my);
    SDL_Window *w = SDL_GetMouseFocus();
    if (w) {
        int ww, wh;
        SDL_GetWindowSize(w, &ww, &wh);
        *x = mx * SCR_W / ww; *y = my * SCR_H / wh;
    }
    return (b & SDL_BUTTON(1)) != 0;
#endif
}
