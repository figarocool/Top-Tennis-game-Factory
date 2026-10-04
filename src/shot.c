/* Stroke families share one coordinate model. Compatibility is checked against
 * a private reference build. Animation identifiers describe externally loaded assets. */
#include "player.h"

enum Family { NONE, SERVE, DRIVE, LOB, DROP, SMASH, VOLLEY };
enum Aim { NATURAL, CROSS, WIDE };
typedef struct { enum Family family; enum Aim aim; int backhand, power; } Stroke;

static Stroke classify(unsigned a)
{
    Stroke s = { NONE, NATURAL, 0, 0 };
    if (a == 23 || (a >= 29 && a <= 33)) {
        s.family = SERVE; s.power = a >= 31;
        s.aim = a == 29 || a == 31 ? CROSS : a == 30 || a == 33 ? WIDE : NATURAL;
    } else if (a == 18 || a == 20 || (a >= 25 && a <= 28) || (a >= 58 && a <= 63)) {
        s.family = DRIVE; s.power = a >= 58;
        s.backhand = a == 20 || a == 27 || a == 28 || a >= 61;
        if (a >= 58) s.aim = (enum Aim)((a - 58) % 3);
        else if (a >= 25) s.aim = a % 2 ? CROSS : WIDE;
    } else if (a >= 34 && a <= 45) {
        s.family = a <= 39 ? LOB : DROP;
        unsigned first = a <= 39 ? 34 : 40;
        unsigned index = a - first;
        s.backhand = index >= 3;
        /* Lob entries order the straight variant between cross and wide. */
        if (s.family == LOB) s.aim = index % 3 == 1 ? NATURAL : index % 3 == 0 ? CROSS : WIDE;
        else s.aim = (enum Aim)(index % 3);
    } else if (a >= 47 && a <= 49) {
        s.family = SMASH; s.aim = (enum Aim)(a - 47);
    } else if ((a >= 51 && a <= 53) || (a >= 55 && a <= 57)) {
        s.family = VOLLEY; s.backhand = a >= 55;
        s.aim = (enum Aim)(a - (s.backhand ? 55 : 51));
    }
    return s;
}

void shot_sound(uint8_t a)
{
    Stroke s = classify(a);
    if (s.family != NONE) snd_play(s.power || s.family == SMASH ? 4 : 3);
}

void ball_shot(TBall *b, uint8_t animation)
{
    Stroke s = classify(animation);
    if (s.family == NONE) return;
    int upper = !b->dir, x = b->x, y, height, speed;
    int cross = upper ? 209 + (209 - x) / 2 : 418 - x;
    int wide = upper ? (x > 209 ? 280 : 140) : (x > 209 ? 310 : 110);
    int depth = upper ? (g_y - 226) / 3 : (g_y - 93) / 2;
    if (s.family == SERVE) {
        int correction = g_amp - 5 - b->h;
        correction = s.power ? correction * 2 : correction / 2;
        if (correction < -20) correction = -20;
        if (correction > 20) correction = 20;
        int left_half = x < 209;
        if (upper) {
            x = s.aim == CROSS ? -116 + 80 * left_half :
                s.aim == WIDE ? 200 + 80 * left_half : 165 + 80 * left_half;
        } else {
            x = s.aim == CROSS ? 120 + 95 * left_half :
                s.aim == WIDE ? 200 + 85 * left_half : 160 + 90 * left_half;
        }
        y = upper ? 122 + correction : 185 - correction;
        height = b->h; speed = s.power ? 3 : 2;
    } else {
        speed = s.family == SMASH || s.family == VOLLEY ? 4 : s.power ? 3 : 2;
        height = s.family == LOB ? 85 : s.family == DROP ? (upper ? 43 : 40) :
                 s.family == DRIVE ? 35 : b->h;
        y = s.family == DROP ? (upper ? 135 : 170) :
            s.family == SMASH || s.family == VOLLEY ? (upper ? 110 : 210) :
            depth + (s.family == LOB ? (upper ? 110 : 210) : (upper ? 115 : 200));
        if (s.family == DROP && !upper) wide = b->x > 209 ? 305 : 115;
        if (s.aim == CROSS) x = cross;
        else if (s.aim == WIDE) x = wide;
        else if (s.family != SMASH) {
            int offset = s.backhand ? 40 : -40;
            if (s.family == LOB || s.family == DROP) {
                if (upper && s.backhand) offset = 50;
            }
            if (!upper) offset = s.backhand ? -40 : 40;
            x += offset;
            if (upper && s.family == DRIVE) {
                if (!s.backhand && b->x > 240) x -= 20;
                if (s.backhand && b->x > 220) x -= 20;
                if (s.backhand && b->x < 180) x += 20;
            }
        }
    }
    b->dir = upper;
    ball_hit(b, speed, height, y, x);
    shot_sound(animation);
}
