#include "opl.h"
#include <math.h>
#include <string.h>

typedef struct {
    int    mult, ksl, tl, ar, dr, sl, rr, ws, ksr, sus, am, vib, egt;
    int    stage;              /* 0 off, 1 attack, 2 decay, 3 sustain, 4 release */
    double env;                /* attenuation in dB, 0 = loudest, 96 = silent */
    double phase;
    double out1, out2;
} Op;

typedef struct { Op op[2]; int fnum, block, keyon, fb, conn; } Chan;

static Chan   ch[9];
static int    regs[256];
static int    srate = 44100;
static const int op_off[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };
static const int ksl_tab[16] = { 0, 32, 40, 45, 48, 51, 53, 55, 56, 58, 59, 60, 61, 62, 63, 64 };
static const double mults[16] = { 0.5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15 };

static int find_op(int off, int *c, int *o)
{
    for (int i = 0; i < 9; i++) {
        if (off == op_off[i]) { *c = i; *o = 0; return 1; }
        if (off == op_off[i] + 3) { *c = i; *o = 1; return 1; }
    }
    return 0;
}

void opl_reset(int rate)
{
    srate = rate;
    memset(ch, 0, sizeof ch);
    memset(regs, 0, sizeof regs);
}

static void keyon(Chan *c, int on)
{
    if (on && !c->keyon) {
        for (int i = 0; i < 2; i++) { c->op[i].stage = 1; c->op[i].phase = 0; c->op[i].out1 = c->op[i].out2 = 0; }
    } else if (!on && c->keyon) {
        for (int i = 0; i < 2; i++) if (c->op[i].stage) c->op[i].stage = 4;
    }
    c->keyon = on;
}

void opl_write(int reg, int val)
{
    reg &= 0xff; val &= 0xff;
    regs[reg] = val;
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
    } else if (reg >= 0xa0 && reg <= 0xa8) {
        ch[reg - 0xa0].fnum = (ch[reg - 0xa0].fnum & 0x300) | val;
    } else if (reg >= 0xb0 && reg <= 0xb8) {
        Chan *k = &ch[reg - 0xb0];
        k->fnum = (k->fnum & 0xff) | (val & 3) << 8;
        k->block = val >> 2 & 7;
        keyon(k, val >> 5 & 1);
    } else if (reg >= 0xc0 && reg <= 0xc8) {
        ch[reg - 0xc0].fb = val >> 1 & 7;
        ch[reg - 0xc0].conn = val & 1;
    }
}

static double wave(int ws, double ph)           /* ph in cycles */
{
    ph -= floor(ph);
    double s = sin(ph * 2 * M_PI);
    switch (ws) {
    case 1: return s > 0 ? s : 0;
    case 2: return fabs(s);
    case 3: return (ph < 0.25 || (ph >= 0.5 && ph < 0.75)) ? fabs(s) : 0;
    }
    return s;
}

static double rate_secs(int r, double rks, int attack)   /* time for the full 96 dB swing */
{
    if (r == 0) return 1e9;
    double eff = r + rks / 4.0;
    if (eff >= 15) return attack ? 0.0 : 0.0024;
    return (attack ? 2.826 : 39.28) / pow(2.0, eff - 1);
}

static double op_run(Chan *c, Op *p, double pm)
{
    if (!p->stage) return 0;
    double rks = ((c->block << 1) | (c->fnum >> 9)) >> (p->ksr ? 0 : 2);
    double dt = 96.0 / srate;                    /* dB per second of rate 1/s */
    switch (p->stage) {
    case 1: {
        double t = rate_secs(p->ar, rks, 1);
        if (t <= 0) p->env = 0;
        else p->env -= (p->env + 6.0) * (3.0 / (t * srate)) * 1.0; /* exponential approach to 0 dB */
        if (p->env <= 0.1) { p->env = 0; p->stage = 2; }
        break; }
    case 2: {
        double t = rate_secs(p->dr, rks, 0);
        p->env += dt / t;
        double sl = p->sl == 15 ? 93 : p->sl * 3;
        if (p->env >= sl) { p->env = sl; p->stage = 3; }
        break; }
    case 3:
        if (!p->egt) { p->stage = 4; }
        break;
    case 4: {
        double t = rate_secs(p->rr, rks, 0);
        p->env += dt / t;
        break; }
    }
    if (p->stage == 4 && !c->keyon) { /* release */ }
    if (p->env >= 96) { p->stage = 0; p->env = 96; return 0; }
    double att = p->env + p->tl * 0.75;
    if (p->ksl) {
        int a = ksl_tab[c->fnum >> 6] - 8 * (7 - c->block);
        static const double kdiv[4] = { 0, 4.0, 2.0, 1.0 };
        if (a > 0) att += a * 0.375 / kdiv[p->ksl];
    }
    if (att >= 96) return 0;
    double freq = c->fnum * 49716.0 / (double)(1 << (20 - c->block)) * mults[p->mult];
    p->phase += freq / srate;
    p->phase -= floor(p->phase);
    double v = wave(p->ws, p->phase + pm);
    return v * pow(10.0, -att / 20.0);
}

void opl_mix(float *out, int n)
{
    for (int i = 0; i < n; i++) {
        double mix = 0;
        for (int k = 0; k < 9; k++) {
            Chan *c = &ch[k];
            if (!c->op[0].stage && !c->op[1].stage) continue;
            double fbm = c->fb ? (c->op[0].out1 + c->op[0].out2) * 0.5 * 4 * M_PI * pow(2.0, c->fb - 7) / (2 * M_PI) : 0;
            double m = op_run(c, &c->op[0], fbm);
            c->op[0].out2 = c->op[0].out1; c->op[0].out1 = m;
            double s;
            if (c->conn) s = m + op_run(c, &c->op[1], 0);
            else s = op_run(c, &c->op[1], m * 2.0);
            mix += s;
        }
        out[i] += (float)(mix * 0.25);
    }
}
