#include "net.h"
#include "platform.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if defined(__vita__)
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#elif defined(__PSP__)
#include <pspkernel.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <psputility.h>
#else
#include <ifaddrs.h>
#endif

/* ------------------------------------------------------------ platform bring-up */

static int net_up;

int net_init(char *err, int errlen)
{
    if (net_up) return 0;
#if defined(__vita__)
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    static SceNetInitParam p;
    p.size = 1024 * 1024;
    p.memory = malloc(p.size);
    p.flags = 0;
    if (!p.memory || sceNetInit(&p) < 0) { snprintf(err, errlen, "CANNOT START THE NETWORK"); return -1; }
    sceNetCtlInit();
    int state = 0;
    sceNetCtlInetGetState(&state);
    if (state != SCE_NETCTL_STATE_CONNECTED) { snprintf(err, errlen, "WI-FI IS NOT CONNECTED"); return -1; }
#elif defined(__PSP__)
    sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    if (sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024) < 0 || sceNetInetInit() < 0 || sceNetApctlInit(0x8000, 48) < 0) {
        snprintf(err, errlen, "CANNOT START THE NETWORK");
        return -1;
    }
    sceNetApctlConnect(1);                         /* first saved network profile */
    int state = 0, t = 0;
    while (t < 300) {                              /* up to 30 s */
        if (sceNetApctlGetState(&state) < 0) break;
        if (state == PSP_NET_APCTL_STATE_GOT_IP) break;
        sceKernelDelayThread(100 * 1000);
        t++;
    }
    if (state != PSP_NET_APCTL_STATE_GOT_IP) { snprintf(err, errlen, "WI-FI NOT AVAILABLE"); return -1; }
#else
    (void)err; (void)errlen;
#endif
    net_up = 1;
    return 0;
}

void net_shutdown(void) { net_close(); }

int net_local_ip(char *buf, int len)
{
    buf[0] = 0;
#if defined(__vita__)
    SceNetCtlInfo info;
    if (sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info) >= 0) snprintf(buf, len, "%s", info.ip_address);
#elif defined(__PSP__)
    union SceNetApctlInfo info;
    if (sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) >= 0) snprintf(buf, len, "%s", info.ip);
#else
    struct ifaddrs *ifa, *p;
    if (getifaddrs(&ifa)) return 0;
    for (p = ifa; p; p = p->ifa_next) {
        if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
        struct sockaddr_in *a = (struct sockaddr_in *)p->ifa_addr;
        if ((ntohl(a->sin_addr.s_addr) >> 24) == 127) continue;
        inet_ntop(AF_INET, &a->sin_addr, buf, len);
        break;
    }
    freeifaddrs(ifa);
#endif
    return buf[0] != 0;
}

uint32_t net_parse_ip(const char *text)
{
    unsigned a, b, c, d;
    char extra;
    if (sscanf(text, "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra) != 4 || a > 255 || b > 255 || c > 255 || d > 255) return 0;
    return a << 24 | b << 16 | c << 8 | d;
}

void net_format_ip(uint32_t a, char *buf)
{
    sprintf(buf, "%u.%u.%u.%u", (unsigned)(a >> 24), (unsigned)(a >> 16 & 255), (unsigned)(a >> 8 & 255), (unsigned)(a & 255));
}

/* ------------------------------------------------------------ sockets */

static int sock = -1;

static int open_socket(int port)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return -1;
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const void *)&one, sizeof one);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&one, sizeof one);
#if defined(SO_NONBLOCK)
    setsockopt(s, SOL_SOCKET, SO_NONBLOCK, (const void *)&one, sizeof one);
#else
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
    if (port) {
        struct sockaddr_in a;
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_port = htons((uint16_t)port);
        a.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
    }
    return s;
}

static void send_to(uint32_t addr, uint16_t port, const void *buf, int len)
{
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(addr);
    sendto(sock, buf, len, 0, (struct sockaddr *)&a, sizeof a);
}

/* returns the length of a datagram (0 = none) and its sender */
static int recv_from(void *buf, int max, uint32_t *addr, uint16_t *port)
{
    struct sockaddr_in a;
    socklen_t l = sizeof a;
    int n = (int)recvfrom(sock, buf, max, 0, (struct sockaddr *)&a, &l);
    if (n <= 0) return 0;
    *addr = ntohl(a.sin_addr.s_addr);
    *port = ntohs(a.sin_port);
    return n;
}

/* ------------------------------------------------------------ protocol */

enum { T_DISCOVER = 'D', T_ANNOUNCE = 'A', T_JOIN = 'J', T_START = 'S', T_READY = 'R', T_GAME = 'G', T_QUIT = 'Q' };

static void put32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static uint32_t get32(const uint8_t *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

static int header(uint8_t *p, int type) { p[0] = 'T'; p[1] = 'T'; p[2] = (uint8_t)type; p[3] = NET_VER; return 4; }
static int valid(const uint8_t *p, int n) { return n >= 4 && p[0] == 'T' && p[1] == 'T' && p[3] == NET_VER; }

static enum { R_NONE, R_HOST, R_GUEST } role;
static uint32_t peer_addr; static uint16_t peer_port;
static NetSettings cfg;
static char my_name[24];
static int  joined, ready;
static uint32_t last_tx;

int net_host_open(const char *name, const NetSettings *s)
{
    net_close();
    sock = open_socket(NET_PORT);
    if (sock < 0) return -1;
    role = R_HOST;
    cfg = *s;
    snprintf(my_name, sizeof my_name, "%s", name);
    joined = ready = 0;
    return 0;
}

static void send_start(void)
{
    uint8_t b[112];
    int n = header(b, T_START);
    b[n++] = cfg.court; b[n++] = cfg.best_of_3; b[n++] = cfg.speed;
    put32(b + n, cfg.seed); n += 4;
    put32(b + n, cfg.check); n += 4;
    memcpy(b + n, cfg.host_name, 24); n += 24;
    memcpy(b + n, cfg.guest_name, 24); n += 24;
    send_to(peer_addr, peer_port, b, n);
}

const NetSettings *net_settings(void) { return &cfg; }

int net_host_tick(void)
{
    uint8_t b[256];
    uint32_t a; uint16_t pt;
    int n;
    while ((n = recv_from(b, sizeof b, &a, &pt)) > 0) {
        if (!valid(b, n)) continue;
        if (b[2] == T_DISCOVER && !joined) {
            uint8_t r[40];
            int l = header(r, T_ANNOUNCE);
            memcpy(r + l, my_name, 24); l += 24;
            send_to(a, pt, r, l);
        } else if (b[2] == T_JOIN && n >= 28 && !joined) {
            joined = 1; peer_addr = a; peer_port = pt;
            snprintf(cfg.guest_name, sizeof cfg.guest_name, "%.23s", (const char *)b + 4);
            send_start();
            last_tx = SDL_GetTicks();
        } else if ((b[2] == T_READY || b[2] == T_GAME) && joined && a == peer_addr) {
            ready = 1;
        }
    }
    if (joined && !ready && SDL_GetTicks() - last_tx > 100) { send_start(); last_tx = SDL_GetTicks(); }
    return ready;
}

/* ---- guest ---- */
static uint32_t scan_t0;

int net_scan_start(void)
{
    net_close();
    sock = open_socket(0);
    if (sock < 0) return -1;
    role = R_GUEST;
    scan_t0 = 0;
    return 0;
}

int net_scan_tick(NetHost *hosts, int *n, int max)
{
    uint32_t now = SDL_GetTicks();
    if (!scan_t0 || now - scan_t0 > 400) {
        uint8_t b[8];
        int l = header(b, T_DISCOVER);
        send_to(0xffffffffu, NET_PORT, b, l);
        char ip[40];                                   /* also the broadcast address of our own /24 network */
        if (net_local_ip(ip, sizeof ip)) {
            uint32_t me = net_parse_ip(ip);
            if (me) send_to((me & 0xffffff00u) | 0xff, NET_PORT, b, l);
        }
        scan_t0 = now ? now : 1;
    }
    uint8_t b[128];
    uint32_t a; uint16_t pt;
    int len;
    while ((len = recv_from(b, sizeof b, &a, &pt)) > 0) {
        if (!valid(b, len) || b[2] != T_ANNOUNCE || len < 28) continue;
        int dup = 0;
        for (int i = 0; i < *n; i++) if (hosts[i].addr == a) dup = 1;
        if (!dup && *n < max) {
            snprintf(hosts[*n].name, 24, "%.23s", (const char *)b + 4);
            hosts[*n].addr = a;
            (*n)++;
        }
    }
    return *n;
}

int net_join_start(uint32_t addr, const char *name)
{
    if (sock < 0 && net_scan_start() < 0) return -1;
    role = R_GUEST;
    peer_addr = addr; peer_port = NET_PORT;
    snprintf(my_name, sizeof my_name, "%s", name);
    joined = ready = 0;
    last_tx = 0;
    return 0;
}

int net_join_tick(NetSettings *s)
{
    uint32_t now = SDL_GetTicks();
    if (!ready && (!last_tx || now - last_tx > 200)) {
        uint8_t b[40];
        int l = header(b, T_JOIN);
        memcpy(b + l, my_name, 24); l += 24;
        send_to(peer_addr, peer_port, b, l);
        last_tx = now ? now : 1;
    }
    uint8_t b[160];
    uint32_t a; uint16_t pt;
    int n;
    while ((n = recv_from(b, sizeof b, &a, &pt)) > 0) {
        if (!valid(b, n) || b[2] != T_START || n < 4 + 3 + 4 + 4 + 48 || a != peer_addr) continue;
        s->court = b[4]; s->best_of_3 = b[5]; s->speed = b[6];
        s->seed = get32(b + 7);
        s->check = get32(b + 11);
        memcpy(s->host_name, b + 15, 24);
        memcpy(s->guest_name, b + 39, 24);
        s->host_name[23] = s->guest_name[23] = 0;
        cfg = *s;
        ready = 1;
        peer_port = pt;
        for (int i = 0; i < 3; i++) { uint8_t r[8]; int l = header(r, T_READY); send_to(peer_addr, peer_port, r, l); }
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------ lockstep */

#define RING 256
static int active, failed, peer_quit;
static const char *fail_msg;
static uint32_t frame;
static uint8_t  loc_s[RING], rem_s[RING];
static int64_t  rem_f[RING];                 /* frame number stored in each ring slot (-1 = empty) */
static uint32_t recv_next;                   /* first remote frame not yet received contiguously */
static uint32_t hash_f[64], hash_v[64];      /* my state hashes, by frame/8 */
static uint32_t peer_hash_f, peer_hash_v;
static unsigned (*read_local)(void);
static uint32_t (*state_hash)(void);
static uint8_t  bits_cur[2];
static uint32_t tx_time;

int net_active(void) { return active; }
int net_is_host(void) { return role == R_HOST; }
int net_peer_quit(void) { return peer_quit; }
const char *net_failure(void) { return failed ? fail_msg : NULL; }
uint32_t net_frames(void) { return frame; }
unsigned net_bits(int side) { return bits_cur[side & 1]; }
void net_set_local_reader(unsigned (*fn)(void), uint32_t (*hash)(void)) { read_local = fn; state_hash = hash; }

void net_begin_match(void)
{
    active = 1; failed = 0; fail_msg = NULL; peer_quit = 0;
    frame = 0; recv_next = 0; tx_time = 0;
    memset(loc_s, 0, sizeof loc_s);
    for (int i = 0; i < RING; i++) rem_f[i] = -1;
    memset(hash_f, 0xff, sizeof hash_f);
    peer_hash_f = 0xffffffffu;
    memset(bits_cur, 0, sizeof bits_cur);
    joined = ready = 1;
}

static void fail(const char *msg) { if (!failed) { failed = 1; fail_msg = msg; peer_quit = 1; } }

static void send_game(void)
{
    uint8_t b[160];
    int n = header(b, T_GAME);
    put32(b + n, frame); n += 4;
    put32(b + n, recv_next); n += 4;
    int k = frame + 1 < 24 ? (int)frame + 1 : 24;
    b[n++] = (uint8_t)k;
    for (int i = 0; i < k; i++) b[n++] = loc_s[(frame - (k - 1) + i) % RING];
    /* latest hash of my own state (frame multiple of 8) */
    uint32_t hf = 0xffffffffu, hv = 0;
    for (int i = 0; i < 64; i++) if (hash_f[i] != 0xffffffffu && (hf == 0xffffffffu || hash_f[i] > hf)) { hf = hash_f[i]; hv = hash_v[i]; }
    put32(b + n, hf); n += 4;
    put32(b + n, hv); n += 4;
    send_to(peer_addr, peer_port, b, n);
    tx_time = SDL_GetTicks();
}

static void handle(const uint8_t *b, int n)
{
    if (!valid(b, n)) return;
    if (b[2] == T_QUIT) { peer_quit = 1; return; }
    if (b[2] == T_START && role == R_GUEST) { for (int i = 0; i < 1; i++) { uint8_t r[8]; int l = header(r, T_READY); send_to(peer_addr, peer_port, r, l); } return; }
    if (b[2] != T_GAME || n < 4 + 9) return;
    uint32_t f = get32(b + 4);
    int k = b[12];
    if (n < 13 + k + 8) return;
    for (int i = 0; i < k; i++) {
        int64_t ff = (int64_t)f - (k - 1) + i;
        if (ff < 0) continue;
        rem_s[ff % RING] = b[13 + i];
        rem_f[ff % RING] = ff;
    }
    while (rem_f[recv_next % RING] == (int64_t)recv_next) recv_next++;
    peer_hash_f = get32(b + 13 + k);
    peer_hash_v = get32(b + 17 + k);
    if (peer_hash_f != 0xffffffffu) {
        int i = (peer_hash_f / 8) % 64;
        if (hash_f[i] == peer_hash_f && hash_v[i] != peer_hash_v) fail("THE GAMES ARE OUT OF SYNC");
    }
}

static void poll_net(void)
{
    uint8_t b[256];
    uint32_t a; uint16_t pt;
    int n;
    while ((n = recv_from(b, sizeof b, &a, &pt)) > 0) if (a == peer_addr) handle(b, n);
}

void net_frame_sync(void)
{
    if (!active || failed) return;
    uint32_t n = frame;
    loc_s[n % RING] = (uint8_t)(read_local ? read_local() : 0);
    if (state_hash && n % 8 == 0) { int i = (n / 8) % 64; hash_f[i] = n; hash_v[i] = state_hash(); }
    send_game();
    int64_t need = (int64_t)n - NET_DELAY;
    uint32_t t0 = SDL_GetTicks();
    if (need >= 0) {
        for (;;) {
            poll_net();
            if (rem_f[need % RING] == need || peer_quit) break;
            plat_poll();
            if (quit_requested) { fail("GAME CLOSED"); break; }
            uint32_t now = SDL_GetTicks();
            if (now - tx_time > 15) send_game();
            if (now - t0 > (n < 4 ? 30000u : 6000u)) { fail("CONNECTION LOST"); break; }
            plat_sleep_ms(1);
        }
    }
    uint8_t mine = need >= 0 ? loc_s[need % RING] : 0;
    uint8_t theirs = (need >= 0 && rem_f[need % RING] == need) ? rem_s[need % RING] : 0;
    if (role == R_HOST) { bits_cur[0] = mine; bits_cur[1] = theirs; }
    else                { bits_cur[0] = theirs; bits_cur[1] = mine; }
    frame++;
}

void net_send_quit(void)
{
    if (sock < 0 || !peer_addr) return;
    for (int i = 0; i < 4; i++) { uint8_t b[8]; int l = header(b, T_QUIT); send_to(peer_addr, peer_port, b, l); }
}

void net_close(void)
{
    if (sock >= 0) close(sock);
    sock = -1;
    active = 0; role = R_NONE; joined = ready = 0;
}
