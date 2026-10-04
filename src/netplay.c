/* Menus and match start for network games (see net.h). */
#include "gamecfg.h"
#include "game.h"
#include "player.h"
#include "sprites.h"
#include "video.h"
#include "text.h"
#include "menu.h"
#include "ui.h"
#include "dialog.h"
#include "options.h"
#include "platform.h"
#include "matchflow.h"
#include "net.h"
#include "dsimg.h"
#include "score.h"
#include "match.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---- helpers used by the lockstep ---- */

static TPlayer *local_player;

static unsigned local_reader(void) { return local_player ? player_read_local(local_player) : 0; }

/* fingerprint of the simulation state, compared between the two machines every 8 frames */
static uint32_t state_hash(void)
{
    uint32_t h = 2166136261u;
#define MIX(v) h = (h ^ (uint32_t)(int32_t)(v)) * 16777619u
    MIX(ball.x); MIX(ball.y); MIX(ball.h); MIX(ball.t); MIX(ball.amp); MIX(ball.bounces); MIX(ball.dir);
    MIX(score.pts[0]); MIX(score.pts[1]);
    TPlayer *ps[2] = { pl_near[0], pl_far[0] };
    for (int i = 0; i < 2; i++) {
        MIX(spr[ps[i]->spr].x); MIX(spr[ps[i]->spr].y); MIX(ps[i]->anim); MIX(ps[i]->frame);
    }
#undef MIX
    return h;
}

void netplay_prepare(void)
{
    local_player = net_is_host() ? pl_near[0] : pl_far[0];
    net_set_local_reader(local_reader, state_hash);
}

/* ---- dialogs ---- */

static int tick_once(void) { return 99; }      /* draws a notice for one frame, then returns while the real work goes on */
static int tick_host(void) { return net_host_tick() ? 99 : 0; }

static NetHost hosts[8];
static int nhosts;
static uint32_t scan_start_ms;
static int tick_scan(void)
{
    net_scan_tick(hosts, &nhosts, 8);
    uint32_t t = SDL_GetTicks() - scan_start_ms;
    return (nhosts && t > 700) || t > 2500 ? 99 : 0;
}

static NetSettings joined_cfg;
static int tick_join(void) { return net_join_tick(&joined_cfg) ? 99 : 0; }

static int run_wait(const char *title, const char *l1, const char *l2, const char *cancel, int (*tick)(void))
{
    Dialog d;
    font_select(8);
    dialog_init(&d, title, 5, 0x1b, 0x60, 0x32);
    dialog_add_label(&d, l1, 0x70, 0x3a);
    if (l2) dialog_add_label(&d, l2, 0x7c, 0x3a);
    dialog_add_button(&d, cancel, 0, 0x8a, 0x84);
    dialog_tick = tick;
    int r = dialog_run(&d);
    dialog_tick = NULL;
    return r;
}

static const char *local_name_default = "";

static int play(MatchSetup *ms, const NetSettings *cfg)
{
    rnd_seed(cfg->seed);
    ms->net = 1;
    int r = matchflow_play(ms, 1);
    (void)r;
    const char *why = net_failure();
    net_close();
    video_fade_out(2);
    if (why) message_box("     NETWORK GAME", why, NULL);
    return 0;
}

static void setup_ms(MatchSetup *ms, const NetSettings *cfg, int host)
{
    match_defaults_from_options(ms);
    int local = ms->ctrl[0] == CTRL_CPU ? 1 : ms->ctrl[0];       /* the player 1 control of the OPTIONS menu */
    ms->doubles = 0;
    for (int i = 0; i < 4; i++) { ms->human[i] = i < 2; ms->cpu_level[i] = 1; ms->ctrl[i] = CTRL_CPU; }
    ms->ctrl[host ? 0 : 1] = local;
    ms->ctrl[host ? 1 : 0] = CTRL_NET;
    ms->court = cfg->court;
    ms->best_of_3 = cfg->best_of_3;
    ms->speed = cfg->speed;
    snprintf(ms->name1, sizeof ms->name1, "%s", cfg->host_name);
    snprintf(ms->name2, sizeof ms->name2, "%s", cfg->guest_name);
}

static void host_game(const char *name)
{
    MatchSetup ms;
    match_defaults_from_options(&ms);
    NetSettings cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.court = (uint8_t)ms.court;
    cfg.best_of_3 = (uint8_t)ms.best_of_3;
    cfg.speed = (uint8_t)ms.speed;
    cfg.seed = timer_ticks() ^ 0x7e57u;
    cfg.check = ball_tables_checksum() ^ ds_checksum();
    snprintf(cfg.host_name, sizeof cfg.host_name, "%s", name);
    if (net_host_open(name, &cfg) < 0) { message_box("     NETWORK GAME", "CANNOT OPEN THE NETWORK PORT", NULL); return; }
    char ip[40], line[64];
    net_local_ip(ip, sizeof ip);
    snprintf(line, sizeof line, "ADDRESS %s", ip[0] ? ip : "?");
    int r = run_wait("   WAITING FOR AN OPPONENT", "OPPONENT: CHOOSE JOIN", line, ds_cstr(0x1c2c), tick_host);
    if (r != 99) { net_close(); return; }
    cfg = *net_settings();
    setup_ms(&ms, &cfg, 1);
    play(&ms, &cfg);
}

static void join_game(const char *name)
{
    static char addr_text[24] = "192.168.1.";
    nhosts = 0;
    if (net_scan_start() < 0) { message_box("     NETWORK GAME", "CANNOT OPEN THE NETWORK", NULL); return; }
    scan_start_ms = SDL_GetTicks();
    if (run_wait("    SEARCHING FOR GAMES", "LOOKING FOR HOSTS ...", NULL, ds_cstr(0x1c2c), tick_scan) == 0) { net_close(); return; }

    uint32_t target = 0;
    static char items[8 + 1][64];
    for (int i = 0; i < nhosts; i++) {
        char ip[24];
        net_format_ip(hosts[i].addr, ip);
        snprintf(items[i], sizeof items[i], "%-8.8s %s", hosts[i].name, ip);
    }
    if (nhosts) {
        Dialog d;
        font_select(8);
        dialog_init(&d, "      GAMES FOUND", 15, 0x1d, 15, 0x2d);
        dialog_add_list(&d, items, nhosts, 10, 25, 0x24, 0x3e);
        dialog_add_button(&d, "@JOIN", 1, 0x8c, 0x3c);
        dialog_add_button(&d, "@ADDRESS", 2, 0x8c, 0x7b);
        dialog_add_button(&d, ds_cstr(0x1c2c), 0, 0x8c, 200);
        int r = dialog_run(&d);
        if (r == 0) { net_close(); return; }
        if (r == 1) target = hosts[d.ctl[0].sel].addr;
    }
    while (!target) {
        char buf[24];
        snprintf(buf, sizeof buf, "%s", addr_text);
        Dialog d;
        font_select(8);
        dialog_init(&d, "     ADDRESS OF THE HOST:", 5, 0x1c, 0x60, 0x32);
        dialog_add_edit(&d, buf, 15, 15, 0x77, 0x50);
        dialog_add_button(&d, ds_cstr(0x3057), 1, 0x8a, 0x46);
        dialog_add_button(&d, ds_cstr(0x1c2c), 0, 0x8a, 0xbe);
        if (!dialog_run(&d)) { net_close(); return; }
        target = net_parse_ip(buf);
        if (target) snprintf(addr_text, sizeof addr_text, "%s", buf);
        else message_box("     NETWORK GAME", "THAT IS NOT AN ADDRESS", NULL);
    }
    memset(&joined_cfg, 0, sizeof joined_cfg);
    net_join_start(target, name);
    int r = run_wait("        CONNECTING", "WAITING FOR THE HOST ...", NULL, ds_cstr(0x1c2c), tick_join);
    if (r != 99) { net_close(); return; }
    if (joined_cfg.check != (ball_tables_checksum() ^ ds_checksum())) {
        net_close();
        message_box("     NETWORK GAME", "GAME VERSIONS DIFFER", NULL);
        return;
    }
    MatchSetup ms;
    setup_ms(&ms, &joined_cfg, 0);
    play(&ms, &joined_cfg);
}

int act_network(Menu *m, MenuItem *it)
{
    (void)m; (void)it;
    char name[24], other[24];
    Dialog d;
    font_select(8);
    dialog_init(&d, "      NETWORK MATCH", 5, 0x1b, 0x60, 0x32);
    dialog_add_label(&d, "  LOCAL NETWORK GAME", 0x75, 0x40);
    dialog_add_button(&d, "@HOST", 1, 0x8a, 0x38);
    dialog_add_button(&d, "@JOIN", 2, 0x8a, 0x7a);
    dialog_add_button(&d, ds_cstr(0x1c2c), 0, 0x8a, 0xbe);
    int r = dialog_run(&d);
    if (r == 0) { ui_dirty = 1; return MR_STAY; }
    MatchSetup probe;
    match_defaults_from_options(&probe);
    if (!ask_names(1, 0, name, other)) { ui_dirty = 1; return MR_STAY; }
    char err[64] = "";
    run_wait("        NETWORK GAME", "STARTING THE NETWORK ...", NULL, ds_cstr(0x1c2c), tick_once);
    if (net_init(err, sizeof err) < 0) {
        message_box("     NETWORK GAME", err, NULL);
        ui_dirty = 1;
        return MR_STAY;
    }
    (void)local_name_default;
    if (r == 1) host_game(name); else join_game(name);
    ui_dirty = 1;
    return MR_STAY;
}
