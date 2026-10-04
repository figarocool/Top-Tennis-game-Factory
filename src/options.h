/* TENNIS.OPT (76 bytes): the menus' selections, the key maps and the joystick calibration.
 * Layout from 1000:b03f (menus -> file) and 1000:b319 (file -> menus); bf04/bfaa read and write the file. */
#ifndef OPTIONS_H
#define OPTIONS_H
#include <stdint.h>

typedef struct {
    uint8_t o[28];         /* [0] singles(1)/doubles(0)  [1] cpu level  [2] 3 sets(1)/5 sets(0)  [3] court
                            * [4..15] machine shots 'A'..'L' or '?'  [16] 0  [17] training side (1=up)  [18] fixed serve
                            * [19] machine delay [20] training court [21] music [22] sfx [23] speed [24..27] player controls */
    uint8_t keys[2][5];    /* up, down, left, right, fire scancodes */
    uint8_t joy[2][19];    /* calibration of joystick 1 and 2 */
} OptFile;

extern OptFile opt;
extern char data_dir[512];

void opt_defaults(void);
void opt_from_menus(OptFile *o);       /* FUN_1000_b03f */
void opt_to_menus(const OptFile *o);   /* FUN_1000_b319 */
int  opt_load(void);                   /* FUN_1000_bfaa: reads TENNIS.OPT (silently ignores a missing file) */
int  opt_save(void);                   /* FUN_1000_bf04 */
#endif
