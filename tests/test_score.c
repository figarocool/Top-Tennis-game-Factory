#include "../src/score.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    TScore s;
    score_init(&s);
    for (int i = 0; i < 3; i++) { score_award(&s, 1); score_award(&s, 2); }
    assert(s.pts[0] == 40 && s.pts[1] == 40);
    assert(!score_award(&s, 1) && s.pts[0] == 50);
    assert(!score_award(&s, 2) && s.pts[0] == 40 && s.pts[1] == 40);
    score_award(&s, 2);
    assert(score_award(&s, 2) == 1 && s.games[1][0] == 1);

    score_init(&s); s.games[0][0] = 5; s.games[1][0] = 6;
    assert(score_game(&s, 1) == 1 && s.tiebreak);
    for (int i = 0; i < 6; i++) assert(!score_award(&s, 1));
    assert(score_award(&s, 1) == 2 && s.set == 2 && !s.tiebreak);

    score_init(&s); s.set = 3; s.sets[0] = s.sets[1] = 1;
    s.games[0][2] = 5; s.games[1][2] = 6;
    assert(score_game(&s, 1) == 1 && !s.tiebreak);
    assert(score_game(&s, 1) == 1 && s.games[0][2] == 7);
    assert(score_game(&s, 1) == 3 && s.games[0][2] == 8);

    score_init(&s); score_set_best_of_3(&s, 0);
    const int winners[] = {1, 2, 1, 2, 1};
    int result = 0;
    for (int set = 0; set < 5; set++)
        for (int game = 0; game < 6; game++)
            for (int point = 0; point < 4; point++) result = score_award(&s, winners[set]);
    assert(result == 3 && s.set == 6 && s.sets[0] == 3 && s.sets[1] == 2);
    assert(!score_change_ends(&s)); /* The fifth set is complete: no sixth-set lookup. */
    puts("scoring: deuce, tiebreak, deciding set and complete five-set match: OK");
    return 0;
}
