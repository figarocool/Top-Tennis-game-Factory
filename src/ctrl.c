/* Compatibility controllers organised around movement, preparation and release
 * states. One court-side policy serves both players. */
#include "player.h"
#include "sprites.h"
#include "platform.h"

enum { UP = 1, DOWN = 2, LEFT = 4, RIGHT = 8, SHOT = 16 };
enum { WAIT_SERVE = 21, TOSS = 22, IDLE = 24 };
static uint16_t command(unsigned animation, int start) { return (uint16_t)(animation * 256 + !!start); }
static int ready(unsigned a) { return a == 17 || a == 19 || a == 46 || a == 50 || a == 54 || a == TOSS; }
static int vertical(unsigned keys, int far)
{
    if (far && (keys & DOWN)) return 2;
    if (keys & UP) return 1;
    return keys & DOWN ? 2 : 0;
}
static unsigned horizontal_run(TPlayer *p, int right)
{
    unsigned walk = right ? 1 : 2, run = walk + 8;
    if (p->anim == run) return run;
    if (p->anim == walk) return timer_elapsed(p->timer_a) >= 393211u ? run : walk;
    timer_start(p->timer_a); return walk;
}
static unsigned prepare(TPlayer *p, const Sprite *s, int far, int centre)
{
    if (g_h >= 61 && (far ? g_y <= 209 : g_y >= 106)) return 46;
    int projected = ball_x_at_y(&ball, s->y);
    int forehand = far ? projected < centre : projected > centre;
    int baseline = far ? s->y < 121 : s->y >= 180;
    if (baseline) timer_start(p->timer_b);
    return baseline ? (forehand ? 17 : 19) : (forehand ? 50 : 54);
}
static unsigned direction_group(unsigned keys)
{
    if (keys & RIGHT) return g_x < 210 ? 2 : 1;
    if (keys & LEFT) return g_x < 209 ? 1 : 2;
    return 0;
}
static unsigned release_ground(TPlayer *p, unsigned keys, int far)
{
    int backhand = p->anim == 19;
    unsigned group = direction_group(keys), v = vertical(keys, far);
    if (v == 2) return 40 + 3 * backhand + (group == 1 ? 2 : group == 2 ? 1 : 0);
    if (v == 1) return 35 + 3 * backhand + (group == 1 ? 1 : group == 2 ? -1 : 0);
    if (timer_elapsed(p->timer_b) >= 786420u) return 58 + 3 * backhand + (group == 1 ? 2 : group == 2 ? 1 : 0);
    return group ? 25 + 2 * backhand + (group == 1) : 18 + 2 * backhand;
}
uint16_t ctrl_human(TPlayer *p, int far)
{
    Sprite *s = spr_get(p->spr); if (!s) return command(IDLE, 1);
    unsigned keys = p->input, a = p->anim, v = vertical(keys, far);
    if (keys & SHOT) {
        if (a == WAIT_SERVE) return command(TOSS, 1);
        if (ready(a)) return command(a, 0);
        return command(prepare(p, s, far, s->x + s->w / 2), 1);
    }
    if (a == WAIT_SERVE) return command(a, 0);
    if (a == TOSS) {
        if (!v) return command(TOSS, 0);
        unsigned serve = v == 1 ? 32 : 23;
        if (keys & LEFT) serve = v == 1 ? 31 : 29;
        else if (keys & RIGHT) serve = v == 1 ? 33 : 30;
        return command(serve, 1);
    }
    if (a == 17 || a == 19) return command(release_ground(p, keys, far), 1);
    if (ready(a)) {
        unsigned group = direction_group(keys);
        return command(a + 1 + (group == 1 ? 2 : group == 2 ? 1 : 0), 1);
    }
    if (!(keys & (LEFT | RIGHT))) return v ? command(v == 1 ? 3 : 4, 1) : command(IDLE, 0);
    int right = !!(keys & RIGHT);
    if (!v) return command(horizontal_run(p, right), 1);
    return command(right ? (v == 1 ? 5 : 6) : (v == 1 ? 8 : 7), 1);
}

static unsigned cpu_toss(const TPlayer *p)
{
    if (!ball.past_apex || (uint16_t)(g_amp - 3) <= (uint16_t)g_h) return TOSS;
    if ((p->cpu & 8) && g_first_serve) {
        unsigned draw = rnd(10);
        return draw == 6 ? 31 : draw == 5 ? 33 : draw == 4 ? 32 : TOSS;
    }
    if (!(p->cpu & 12)) return 23;
    unsigned draw = rnd(3); return draw == 2 ? 29 : draw == 1 ? 30 : 23;
}
static uint16_t cpu_release(TPlayer *p, const Sprite *s, int far)
{
    unsigned a = p->anim;
    int ground = a == 17 || a == 19;
    unsigned basic = ground ? (a == 19 ? 20 : 18) : a + 1;
    if (g_out_flag) return command(basic, 1);
    int margin = ground ? (far ? 11 : 15) : a == 46 ? 15 : 12;
    int close = far ? g_y < s->y + margin : g_y > s->y - margin;
    if (!close) return command(a, 0);
    int central = far ? s->x > 129 && s->x < 281 : s->x > 99 && s->x < 311;
    if (!(p->cpu & 3)) return command(central ? basic : ground ? (a == 19 ? 28 : 26) : basic + 2, 1);
    if (!ground) {
        unsigned draw = rnd(3); return command(basic + (draw == 2 ? 1 : draw == 1 ? 2 : 0), 1);
    }
    int late = (p->cpu & 2) && timer_elapsed(p->timer_b) >= 786421u;
    unsigned draw = rnd(15), hand = a == 19;
    if (draw >= 12) return command(40 + 3 * hand + (14 - draw), 1);
    unsigned aim = draw >= 8 ? 1 : draw >= 4 ? 2 : 0;
    if (late) return command(58 + hand * 3 + aim, 1);
    return command(aim ? 24 + hand * 2 + aim : 18 + hand * 2, 1);
}
static uint16_t recover(const Sprite *s, int far)
{
    int x = 190;
    if (far ? s->y >= 101 : s->y < 190) {
        int low = far ? 140 : 160, high = far ? 280 : 250;
        x = g_first_x1 < low ? 150 : g_first_x1 < high ? 190 : 220;
    }
    int centre_band = far ? s->y >= 101 && s->y <= 144 : s->y > 162 && s->y < 220;
    int step = s->x < x - 10 ? 1 : s->x > x + 10 ? -1 : 0;
    unsigned animation = IDLE;
    if (centre_band) animation = step > 0 ? (far ? 6 : 5) : step < 0 ? (far ? 7 : 8) : far ? 4 : 3;
    else if (step) animation = step > 0 ? 1 : 2;
    else if (far ? s->y < 90 : s->y > 240) animation = far ? 4 : 3;
    return command(animation, animation != IDLE);
}
static uint16_t cpu(TPlayer *p, int far)
{
    Sprite *s = spr_get(p->spr); if (!s) return command(IDLE, 0);
    if (p->anim == WAIT_SERVE) return ball.bounces >= 2 ? command(TOSS, 1) : command(WAIT_SERVE, 0);
    if (p->anim == TOSS) { unsigned a = cpu_toss(p); return command(a, a != TOSS); }
    if (ready(p->anim)) return cpu_release(p, s, far);
    if (g_out_flag) return command(IDLE, 0);
    int receiving = ball.dir == far && (far ? g_y < 200 : g_y > 115);
    if (!receiving) return !g_rally && ball.dir != far ? recover(s, far) : command(IDLE, 0);
    int special = !g_doubles && !g_rally && (far ? s->y < 110 && g_y1 > 120 && g_y1 < 150 :
                                          g_y_pred < s->y && g_y1 > 150 && g_y1 < 190);
    int target = ball_x_at_y(&ball, s->y), right = s->x + 5 < target || s->x < 21;
    int aligned = right && (target < s->x + 51 || s->x > 336);
    if (!aligned) {
        if (special) return command(right ? (far ? 6 : 5) : (far ? 7 : 8), 1);
        if (p->anim == (right ? 1 : 2) || p->anim == (right ? 9 : 10) ||
            !g_doubles || !g_rally || (far ? s->y < 121 : s->y > 179)) return command(horizontal_run(p, right), 1);
        return command(IDLE, 0);
    }
    int arriving = far ? g_y < s->y + 40 || (s->y > 120 && g_y < 190) :
                         g_y > s->y - 40 || (s->y < 180 && g_y > 115);
    if (arriving) {
        int reachable = far ? g_y > s->y - 1 : g_y < s->y + 1;
        if (reachable && ball.bounces < 2) return command(prepare(p, s, far, s->x + 28), 1);
        return command(IDLE, 0);
    }
    if (special) return command(far ? 4 : 3, 1);
    int approach = far ? g_y < (g_doubles ? 123 : 190) : g_y > (g_doubles ? 175 : 150);
    if (!approach) return command(IDLE, 0);
    int baseline = far ? s->y < 121 : s->y >= 180;
    int short_ball = !ball.bounces && (far ? g_y1 < s->y + 10 : g_y1 > s->y - 10);
    unsigned a;
    if (baseline && short_ball) a = far ? 3 : 4;
    else {
        int forehand = far ? target < s->x + 28 : target > s->x + 28;
        a = baseline ? (forehand ? 17 : 19) : (forehand ? 50 : 54);
        if (baseline && !far) timer_start(p->timer_b);
    }
    if (far) timer_start(p->timer_b);
    return command(a, 1);
}
uint16_t ctrl_cpu_down(TPlayer *p) { return cpu(p, 0); }
uint16_t ctrl_cpu_up(TPlayer *p) { return cpu(p, 1); }
