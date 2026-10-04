/* Small software OPL2 (YM3812): 9 two-operator voices. Single precision and lookup tables only, so it is cheap
 * enough for handhelds without a double-precision FPU. Envelope times and levels are approximations of the chip. */
#include "opl.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#define SINE_BITS 10
#define SINE_N    (1 << SINE_BITS)

typedef struct {
    int   mult, ksl, tl, ar, dr, sl, rr, ws, ksr, am, vib, egt;
    int   stage;                 /* 0 off, 1 attack, 2 decay, 3 sustain, 4 release */
    float env;                   /* attenuation in dB, 0 = loudest, 96 = silent */
    float att_k, dec_step, rel_step, sl_db, level_db;     /* per-sample values, refreshed by op_update */
    uint32_t phase, phase_inc;   /* 32-bit phase accumulator */
    float out1, out2;
} Op;

typedef struct { Op op[2]; int fnum, block, keyon, fb, conn; float fb_scale; } Chan;

static Chan  ch[9];
static int   srate = 44100;
static float sine_tab[4][SINE_N];
static float amp_tab[1024];      /* 10^(-i/10/20): amplitude for an attenuation of i/10 dB */
static int   tables_done;
static const int op_off[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };
static const int ksl_tab[16] = { 0, 32, 40, 45, 48, 51, 53, 55, 56, 58, 59, 60, 61, 62, 63, 64 };
static const float mults[16] = { 0.5f, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15 };

static void make_tables(void)
{
    for (int i = 0; i < SINE_N; i++) {
        float s = sinf((float)i * (2.0f * 3.14159265f / SINE_N));
        sine_tab[0][i] = s;
        sine_tab[1][i] = s > 0 ? s : 0;
        sine_tab[2][i] = fabsf(s);
        sine_tab[3][i] = (i & (SINE_N / 2 - 1)) < SINE_N / 4 ? fabsf(s) : 0;
    }
    for (int i = 0; i < 1024; i++) amp_tab[i] = powf(10.0f, -(float)i / 200.0f);
    tables_done = 1;
}

static int find_op(int off, int *c, int *o)
{
    for (int i = 0; i < 9; i++) {
        if (off == op_off[i]) { *c = i; *o = 0; return 1; }
        if (off == op_off[i] + 3) { *c = i; *o = 1; return 1; }
    }
    return 0;
}

static float rate_secs(int r, float rks, int attack)     /* time for the full 96 dB swing */
{
    if (r == 0) return 1e9f;
    float eff = (float)r + rks / 4.0f;
    if (eff >= 15) return attack ? 0.0f : 0.0024f;
    return (attack ? 2.826f : 39.28f) / exp2f(eff - 1);
}

/* recompute the per-sample constants of one operator (parameters, fnum or block changed) */
static void op_update(Chan *c, Op *p)
{
    float rks = (float)(((c->block << 1) | (c->fnum >> 9)) >> (p->ksr ? 0 : 2));
    float t = rate_secs(p->ar, rks, 1);
    p->att_k = t <= 0 ? 0.0f : 3.0f / (t * srate);
    p->dec_step = 96.0f / (rate_secs(p->dr, rks, 0) * srate);
    p->rel_step = 96.0f / (rate_secs(p->rr, rks, 0) * srate);
    p->sl_db = p->sl == 15 ? 93.0f : (float)p->sl * 3.0f;
    float att = (float)p->tl * 0.75f;
    if (p->ksl) {
        static const float kdiv[4] = { 0, 4.0f, 2.0f, 1.0f };
        int a = ksl_tab[c->fnum >> 6] - 8 * (7 - c->block);
        if (a > 0) att += (float)a * 0.375f / kdiv[p->ksl];
    }
    p->level_db = att;
    float freq = (float)c->fnum * 49716.0f / (float)(1 << (20 - c->block)) * mults[p->mult];
    p->phase_inc = (uint32_t)(freq / (float)srate * 4294967296.0f);
}

static void chan_update(Chan *c) { op_update(c, &c->op[0]); op_update(c, &c->op[1]); }

void opl_reset(int rate)
{
    srate = rate;
    if (!tables_done) make_tables();
    memset(ch, 0, sizeof ch);
}

static void keyon(Chan *c, int on)
{
    if (on && !c->keyon) {
        for (int i = 0; i < 2; i++) { Op *p = &c->op[i]; p->stage = 1; p->env = 96.0f; p->phase = 0; p->out1 = p->out2 = 0; }
    } else if (!on && c->keyon) {
        for (int i = 0; i < 2; i++) if (c->op[i].stage) c->op[i].stage = 4;
    }
    c->keyon = on;
}

void opl_write(int reg, int val)
{
    reg &= 0xff; val &= 0xff;
    int c, o;
    int grp = reg & 0xe0;
    if (reg >= 0x20 && reg <= 0xf5 && (grp == 0x20 || grp == 0x40 || grp == 0x60 || grp == 0x80 || grp == 0xe0) &&
        find_op(reg & 0x1f, &c, &o)) {
        Op *p = &ch[c].op[o];
        switch (grp) {
        case 0x20: p->am = val >> 7 & 1; p->vib = val >> 6 & 1; p->egt = val >> 5 & 1; p->ksr = val >> 4 & 1; p->mult = val & 15; break;
        case 0x40: p->ksl = val >> 6; p->tl = val & 63; break;
        case 0x60: p->ar = val >> 4; p->dr = val & 15; break;
        case 0x80: p->sl = val >> 4; p->rr = val & 15; break;
        case 0xe0: p->ws = val & 3; break;
        }
        op_update(&ch[c], p);
    } else if (reg >= 0xa0 && reg <= 0xa8) {
        ch[reg - 0xa0].fnum = (ch[reg - 0xa0].fnum & 0x300) | val;
        chan_update(&ch[reg - 0xa0]);
    } else if (reg >= 0xb0 && reg <= 0xb8) {
        Chan *k = &ch[reg - 0xb0];
        k->fnum = (k->fnum & 0xff) | (val & 3) << 8;
        k->block = val >> 2 & 7;
        chan_update(k);
        keyon(k, val >> 5 & 1);
    } else if (reg >= 0xc0 && reg <= 0xc8) {
        ch[reg - 0xc0].fb = val >> 1 & 7;
        ch[reg - 0xc0].fb_scale = ldexpf(1.0f, ch[reg - 0xc0].fb - 7);
        ch[reg - 0xc0].conn = val & 1;
    }
}

/* one operator sample; pm is the phase modulation in cycles */
static inline float op_run(Chan *c, Op *p, float pm)
{
    (void)c;
    switch (p->stage) {
    case 1:
        if (p->att_k == 0) p->env = 0;
        else p->env -= (p->env + 6.0f) * p->att_k;
        if (p->env <= 0.1f) { p->env = 0; p->stage = 2; }
        break;
    case 2:
        p->env += p->dec_step;
        if (p->env >= p->sl_db) { p->env = p->sl_db; p->stage = 3; }
        break;
    case 3:
        if (!p->egt) p->stage = 4;
        break;
    case 4:
        p->env += p->rel_step;
        break;
    default:
        return 0;
    }
    if (p->stage >= 2 && p->env >= 96.0f) { p->stage = 0; p->env = 96.0f; return 0; }
    float att = p->env + p->level_db;
    p->phase += p->phase_inc;
    if (att >= 96.0f) return 0;
    if (att < 0) att = 0;
    /* phase offset in 1/2^32 cycles, through a 32-bit float->int conversion (an int64 one is a slow library call on the PSP) */
    uint32_t ph = p->phase + ((uint32_t)(int32_t)(pm * 536870912.0f) << 3);
    return sine_tab[p->ws][ph >> (32 - SINE_BITS)] * amp_tab[(int)(att * 10.0f)];
}

void opl_mix(float *out, int n)
{
    for (int i = 0; i < n; i++) {
        float mix = 0;
        for (int k = 0; k < 9; k++) {
            Chan *c = &ch[k];
            if (!c->op[0].stage && !c->op[1].stage) continue;
            float fbm = c->fb ? (c->op[0].out1 + c->op[0].out2) * c->fb_scale : 0;
            float m = op_run(c, &c->op[0], fbm);
            c->op[0].out2 = c->op[0].out1; c->op[0].out1 = m;
            mix += c->conn ? m + op_run(c, &c->op[1], 0) : op_run(c, &c->op[1], m * 2.0f);
        }
        out[i] += mix * 0.25f;
    }
}
