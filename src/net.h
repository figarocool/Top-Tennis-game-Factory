/* Network play over a local network (UDP), PC / PS Vita / PSP in any combination.
 *
 * Both machines run the whole match; every frame they swap the two players' button states ("lockstep"), so the
 * simulation stays identical. The host plays the near side, the guest the far side. */
#ifndef NET_H
#define NET_H
#include <stdint.h>

#define NET_PORT   5757
#define NET_VER    3
#define NET_DELAY  2                  /* input delay in frames (hides the network latency) */

typedef struct {
    uint8_t  court, best_of_3, speed;
    uint32_t seed, check;
    char     host_name[24], guest_name[24];
} NetSettings;

typedef struct { char name[24]; uint32_t addr; } NetHost;

int  net_init(char *err, int errlen);         /* brings the network up (Wi-Fi on the consoles) */
void net_shutdown(void);
int  net_local_ip(char *buf, int len);        /* "192.168.1.5" or "" */

/* host */
int  net_host_open(const char *name, const NetSettings *s);
int  net_host_tick(void);
const NetSettings *net_settings(void);        /* settings of the current game (the host learns the guest name from JOIN) */                     /* 1 once a guest joined and is ready */
/* guest */
int  net_scan_start(void);
int  net_scan_tick(NetHost *hosts, int *n, int max);   /* call repeatedly; collects announcements */
int  net_join_start(uint32_t addr, const char *name);
int  net_join_tick(NetSettings *s);           /* 1 once the host sent the match settings */
uint32_t net_parse_ip(const char *text);      /* 0 if invalid */
void net_format_ip(uint32_t addr, char *buf);

/* during the match */
int  net_active(void);
int  net_is_host(void);
void net_begin_match(void);                   /* frame counter = 0 */
void net_set_local_reader(unsigned (*fn)(void), uint32_t (*hash)(void));
void net_frame_sync(void);                    /* once per simulated frame, before the players are updated */
unsigned net_bits(int side);                  /* 0 = near (host), 1 = far (guest): buttons for this frame */
void net_send_quit(void);
int  net_peer_quit(void);
const char *net_failure(void);                /* NULL while everything is fine */
void net_close(void);
uint32_t net_frames(void);
#endif
