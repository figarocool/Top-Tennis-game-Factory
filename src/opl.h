#ifndef OPL_H
#define OPL_H
/* Small software OPL2 (YM3812) used for the music; 9 two-operator voices, floating point. */
void opl_reset(int rate);
void opl_write(int reg, int val);
void opl_mix(float *out, int n);        /* adds n mono samples (range about +-1) to out */
#endif
