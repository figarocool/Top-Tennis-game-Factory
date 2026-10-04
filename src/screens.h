#ifndef SCREENS_H
#define SCREENS_H
#include "gfx.h"

void screen_show_image(const char *pbm);                 /* draw a full-screen PBM into the page */
void screen_intro(void);                                 /* FUN_1000_e520: logo + title screens */
int  wait_vsyncs_or_key(int n);                          /* n vertical retraces, or until a key is pressed (returns 1 on key) */
void notice_missing_data(const char *dir, const char *reason);
#endif
