#include "score.h"
#include <string.h>

TScore score;

void score_init(TScore *s)
{
    memset(s->name, 0, sizeof s->name);
    memset(s->pts, 0, sizeof s->pts);
    memset(s->games, 0, sizeof s->games);
    memset(s->sets, 0, sizeof s->sets);
    s->set = 1;
    s->best_of_3 = 1;
    s->server_flag = 1;
    s->tiebreak = 0;
    s->last = 1;
}

void score_set_names(TScore *s, const char *p1, const char *p2)
{
    /* the original copies the second argument to name 1 ("ANDREW SEAGAL" first) */
    strncpy(s->name[0], p1, 20); s->name[0][20] = 0;
    strncpy(s->name[1], p2, 20); s->name[1][20] = 0;
}

void score_set_best_of_3(TScore *s, int v) { s->best_of_3 = v; }

int score_set(TScore *s, int p)
{
    if (p != 1 && p != 2) return 0;
    s->sets[p - 1]++;
    s->set++;
    int need2 = s->best_of_3 != 0;
    if ((!s->best_of_3 || s->sets[p - 1] < 2) && (s->best_of_3 || s->sets[p - 1] < 3)) return 0;
    (void)need2;
    return 1;
}

int score_game(TScore *s, int p)
{
    if (p != 1 && p != 2) return 1;
    int q = 3 - p;
    int result = 1;
    s->server_flag = s->server_flag == 0;
    uint8_t *mine = &s->games[p - 1][s->set - 1], *opp = &s->games[q - 1][s->set - 1];
    if (*mine <= 4) {
        (*mine)++;
    } else if (*mine == 5) {
        (*mine)++;
        if (*opp < 5) {
            result = score_set(s, p) == 0 ? 2 : 3;
        } else if (*opp == 6 && ((s->best_of_3 && s->set < 3) || (!s->best_of_3 && s->set < 5))) {
            s->tiebreak = 1;
        }
    } else {
        (*mine)++;
        if ((s->tiebreak == 0) + *opp < *mine) {
            if (s->tiebreak) s->tiebreak = 0;
            result = score_set(s, p) == 0 ? 2 : 3;
        }
    }
    s->pts[0] = s->pts[1] = 0;
    return result;
}

int score_point(TScore *s, int p)
{
    if (p != 1 && p != 2) p = 1;
    uint8_t *mine = &s->pts[p - 1], *opp = &s->pts[2 - p];
    int r = 0;
    if (!s->tiebreak) {
        switch (*mine) {
        case 0: *mine = 15; break;
        case 15: *mine = 30; break;
        case 30: *mine = 40; break;
        case 40:
            if (*opp == 40) *mine = 50;            /* advantage */
            else if (*opp == 50) *opp = 40;        /* back to deuce */
            else r = score_game(s, p);
            break;
        case 50: r = score_game(s, p); break;
        }
    } else {
        (*mine)++;
        if (*mine > 6 && *opp + 1 < *mine) r = score_game(s, p);
    }
    return r;
}

/* change of ends: after every odd game (or every 6 points in a tiebreak) */
int score_change_ends(const TScore *s)
{
    if (!s->tiebreak) return (s->games[0][s->set - 1] + s->games[1][s->set - 1]) & 1;
    int t = s->pts[0] + s->pts[1];
    return !(t == 0 || t % 6 != 0);
}

int score_odd(const TScore *s) { return (s->pts[0] + s->pts[1]) & 1; }

int score_award(TScore *s, int p)
{
    if (p == 1 || p == 2) s->last = p;
    return score_point(s, p == 2 ? 2 : 1);
}

const char *score_last_name(const TScore *s) { return s->name[s->last - 1]; }

/* sound id announcing the current score from the server's point of view (SC_xxxx.SND, ids 10..26) */
int score_announce_id(const TScore *s)
{
    int a, b;
    if (!s->server_flag) { a = s->pts[1]; b = s->pts[0]; } else { a = s->pts[0]; b = s->pts[1]; }
    static const struct { int a, b, id; } t[] = {
        {0,15,10},{0,30,11},{0,40,12},{15,0,13},{15,15,14},{15,30,15},{15,40,16},{30,0,17},{30,15,18},
        {30,30,19},{30,40,20},{40,0,21},{40,15,22},{40,30,23},{40,40,24},{50,40,25},{40,50,26},
    };
    for (unsigned i = 0; i < sizeof t / sizeof t[0]; i++)
        if (t[i].a == a && t[i].b == b) return t[i].id;
    return 0;
}
