#ifndef HOF_H
#define HOF_H
#include "menu.h"

void hof_load(void);                                   /* 1000:1281 (TENNIS.HAL, 7 records of 83 bytes) */
int  act_hall_of_fame(Menu *m, MenuItem *it);          /* 1000:cd9e */
void hof_offer_save(const char *desc, const char *extra);   /* 1000:1898: "SAVE RESULT IN HALL OF FAME?" */
#endif
