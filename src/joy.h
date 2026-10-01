#ifndef JOY_H
#define JOY_H
#include "player.h"
#include "menu.h"

struct Joystick { int index; void *dev; };

Joystick *joy_open(int n);                  /* n = 1 or 2; NULL when that joystick is not connected (1020:0c4e) */
unsigned  joy_bits(Joystick *j);            /* bit0 left, 1 right, 2 up, 3 down, 4 button 1, 5 button 2 */
int       joy_present(int n);
void      joy_script_tick(int frame);       /* test hook (TT_VJOY) */
int act_calibrate_j1(Menu *m, MenuItem *it);
int act_calibrate_j2(Menu *m, MenuItem *it);
#endif
