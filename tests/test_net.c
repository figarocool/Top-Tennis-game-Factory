/* Two local peers verify protocol negotiation and deterministic input exchange. */
#include "../src/net.h"
#include "../src/platform.h"
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int quit_requested;
void plat_poll(void) {}
void plat_sleep_ms(int ms) { SDL_Delay((uint32_t)ms); }
static unsigned buttons(void) { return net_is_host() ? 8 : 4; }
static uint32_t state(void) { return net_frames() * 13u; }

int main(void)
{
    assert(SDL_Init(SDL_INIT_TIMER) == 0);
    NetSettings settings;
    memset(&settings, 0, sizeof settings);
    settings.court = 3; settings.best_of_3 = 1; settings.speed = 2;
    settings.seed = 123; settings.check = 456;
    snprintf(settings.host_name, sizeof settings.host_name, "TEST HOST");
    assert(net_host_open("TEST HOST", &settings) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        net_close();
        assert(net_join_start(net_parse_ip("127.0.0.1"), "TEST GUEST") == 0);
        uint32_t start = SDL_GetTicks();
        while (!net_join_tick(&settings)) { assert(SDL_GetTicks() - start < 3000); SDL_Delay(1); }
        assert(settings.court == 3 && settings.check == 456 && settings.seed == 123);
    } else {
        uint32_t start = SDL_GetTicks();
        while (!net_host_tick()) { assert(SDL_GetTicks() - start < 3000); SDL_Delay(1); }
    }
    net_set_local_reader(buttons, state);
    net_begin_match();
    for (int i = 0; i < 64; i++) {
        net_frame_sync();
        assert(!net_failure());
        if (i >= NET_DELAY) assert(net_bits(0) == 8 && net_bits(1) == 4);
        SDL_Delay(1);
    }
    net_close();
    if (child) {
        int status;
        assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
        puts("two-peer negotiation and 64 lockstep frames: OK");
    }
    SDL_Quit();
    return 0;
}
