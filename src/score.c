/* Tennis scoring with the external point representation used by the HUD and saves. */
#include "score.h"
#include <string.h>

TScore score;

void score_init(TScore *s)
{
    memset(s, 0, sizeof *s);
    s->set = s->best_of_3 = s->server_flag = s->last = 1;
}

void score_set_names(TScore *s, const char *first, const char *second)
{
    const char *names[2] = {first, second};
    for (unsigned i = 0; i < 2; i++) {
        size_t n = strlen(names[i]);
        if (n > 20) n = 20;
        memset(s->name[i], 0, sizeof s->name[i]);
        memcpy(s->name[i], names[i], n);
    }
}
void score_set_best_of_3(TScore *s, int enabled) { s->best_of_3 = enabled; }

int score_set(TScore *s, int player)
{
    if (player < 1 || player > 2) return 0;
    unsigned won = ++s->sets[player - 1];
    s->set++;
    return won >= (s->best_of_3 ? 2u : 3u);
}

int score_game(TScore *s, int player)
{
    if (player < 1 || player > 2) return 1;
    if (s->set < 1 || s->set > 5) return 3;
    unsigned winner = player - 1, loser = winner ^ 1, set = s->set - 1;
    unsigned before = s->games[winner][set], other = s->games[loser][set];
    s->games[winner][set]++;
    s->server_flag = !s->server_flag;
    memset(s->pts, 0, sizeof s->pts);
    int won = before == 5 ? other < 5 : before > 5 && s->games[winner][set] > other + !s->tiebreak;
    if (won) {
        if (before > 5) s->tiebreak = 0;
        return score_set(s, player) ? 3 : 2;
    }
    unsigned deciding = s->best_of_3 ? 3 : 5;
    if (before == 5 && other == 6 && s->set < deciding) s->tiebreak = 1;
    return 1;
}

static int point_rank(unsigned points)
{
    if (!points) return 0;
    if (points == 15) return 1;
    if (points == 30) return 2;
    return points == 40 ? 3 : -1;
}

int score_point(TScore *s, int player)
{
    unsigned winner = player == 2, loser = winner ^ 1;
    if (s->tiebreak) {
        s->pts[winner]++;
        return s->pts[winner] >= 7 && s->pts[winner] > s->pts[loser] + 1 ? score_game(s, winner + 1) : 0;
    }
    int rank = point_rank(s->pts[winner]);
    if (rank >= 0 && rank < 3) {
        s->pts[winner] = rank < 2 ? (rank + 1) * 15 : 40;
        return 0;
    }
    if (s->pts[winner] == 50) return score_game(s, winner + 1);
    if (rank != 3) return 0;
    if (s->pts[loser] == 50) s->pts[loser] = 40;
    else if (s->pts[loser] == 40) s->pts[winner] = 50;
    else return score_game(s, winner + 1);
    return 0;
}

int score_change_ends(const TScore *s)
{
    if (s->set < 1 || s->set > 5) return 0;
    if (!s->tiebreak) return (s->games[0][s->set - 1] + s->games[1][s->set - 1]) % 2;
    unsigned played = s->pts[0] + s->pts[1];
    return played && played % 6 == 0;
}
int score_odd(const TScore *s) { return (s->pts[0] + s->pts[1]) % 2; }
int score_award(TScore *s, int player)
{
    if (player == 1 || player == 2) s->last = player;
    return score_point(s, player);
}
const char *score_last_name(const TScore *s) { return s->name[s->last - 1]; }

int score_announce_id(const TScore *s)
{
    unsigned server = !s->server_flag, receiver = server ^ 1;
    unsigned a = s->pts[server], b = s->pts[receiver];
    if (a == 50 && b == 40) return 25;
    if (a == 40 && b == 50) return 26;
    int ar = point_rank(a), br = point_rank(b);
    return ar < 0 || br < 0 || (!ar && !br) ? 0 : 9 + ar * 4 + br;
}
