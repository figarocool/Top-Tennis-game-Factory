#ifndef SCORE_H
#define SCORE_H
#include <stdint.h>

/* TScore (DS:957C). Points are stored as 0, 15, 30, 40, 50 (= advantage); tiebreak points as counts. */
typedef struct {
    char    name[2][21];        /* +0x00, +0x15 (string[20]-like, NUL terminated here) */
    uint8_t pts[2];             /* +0x2a +0x2b */
    uint8_t games[2][5];        /* +0x2c.. p1 sets 1..5 ; +0x31.. p2 */
    uint8_t sets[2];            /* +0x36 +0x37 sets won */
    uint8_t set;                /* +0x38 current set (1..5) */
    uint8_t last;               /* +0x39 player who won the last point (1/2) */
    uint8_t best_of_3;          /* +0x3a */
    uint8_t server_flag;        /* +0x3b toggles every game: which name row carries the serve marker */
    uint8_t tiebreak;           /* +0x3c */
} TScore;

extern TScore score;            /* DS:957C */

void    score_init(TScore *s);                       /* 1000:053a */
void    score_set_names(TScore *s, const char *p1, const char *p2);   /* 1000:05b2 */
void    score_set_best_of_3(TScore *s, int v);       /* 1000:05e8 */
int     score_point(TScore *s, int p);               /* 1000:0600 -> 0 point, 1 game, 2 set, 3 match */
int     score_game(TScore *s, int p);                /* 1000:0768 */
int     score_set(TScore *s, int p);                 /* 1000:0974 */
int     score_change_ends(const TScore *s);          /* 1000:09e6 */
int     score_odd(const TScore *s);                  /* 1000:0a6b */
int     score_award(TScore *s, int p);               /* 1000:0a99 */
const char *score_last_name(const TScore *s);        /* 1000:108e */
int     score_announce_id(const TScore *s);          /* 1000:10df (sound id or 0) */
#endif
