/* Local integration test: needs user-supplied files, never committed fixtures. */
#include "../src/gamecfg.h"
#include "../src/dsimg.h"
#include "../src/options.h"
#include "../src/ui.h"
#include "../src/matchflow.h"
#include "../src/match.h"
#include "../src/platform.h"
#include "../src/video.h"
#include "../src/player.h"
#include "../src/sprites.h"
#include "../src/pak.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 4) { fprintf(stderr, "usage: test_engine data_directory court doubles\n"); return 2; }
    snprintf(data_dir, sizeof data_dir, "%s", argv[1]);
    if (game_boot(argv[1])) return 1;
    assert(ds_size == 19876 && pak_count() == 644);
    cursor_init(); opt_defaults(); ui_build();
    game_make_sprites();
    int16_t *recordings[6] = {ball.rec_ball, ball.rec_shadow, players[0].rec,
                            players[1].rec, players[2].rec, players[3].rec};
    game_make_sprites();
    assert(ball.rec_ball == recordings[0] && ball.rec_shadow == recordings[1]);
    for (int i = 0; i < 4; i++) assert(players[i].rec == recordings[i + 2]);
    rnd_seed(13);
    MatchSetup settings;
    memset(&settings, 0, sizeof settings);
    for (int i = 0; i < 4; i++) { settings.ctrl[i] = CTRL_CPU; settings.cpu_level[i] = 4; }
    settings.court = atoi(argv[2]); settings.doubles = atoi(argv[3]);
    settings.best_of_3 = 1; settings.speed = 2;
    snprintf(settings.name1, sizeof settings.name1, "TEST ONE");
    snprintf(settings.name2, sizeof settings.name2, "TEST TWO");
    matchflow_play(&settings, 0);
    assert(score.set >= 1 && score.set <= 5);
    unsigned crossings = 0;
    for (unsigned i = 1; i < g_rec_count && i < 0x2aa4; i++) {
        int a = ball.rec_shadow[(i - 1) * 3 + 1], b = ball.rec_shadow[i * 3 + 1];
        if ((a < 153) != (b < 153)) crossings++;
    }
    assert(score.games[0][0] || score.games[1][0] || score.set > 1 ||
           score.pts[0] + score.pts[1] >= 30 || crossings >= 4);
    printf("court=%d doubles=%d games=%u/%u points=%u/%u set=%u recorded=%u crossings=%u\n", settings.court, settings.doubles,
           score.games[0][0], score.games[1][0], score.pts[0], score.pts[1], score.set, g_rec_count, crossings);
    assert(g_rec_count < 0x2aa4);
    /* Recorded sprites and ball coordinates survive replay without an EXE. */
    g_rec_idx = 0;
    ball_record(&ball); player_record(pl_near[0]); player_record(pl_far[0]);
    int frame = 0;
    int16_t *entry = ball.rec_ball + frame * 3;
    ball_replay(&ball, frame);
    assert(spr[ball.spr_ball].x == entry[0]);
    player_replay(pl_near[0], frame);
    player_replay(pl_far[0], frame);
    /* Options keep their existing file format and can be saved in the new folder. */
    assert(opt_save() == 0);
    assert(opt_load() == 0);
    video_shutdown(); plat_quit(); pak_close(); ds_unload();
    return 0;
}
