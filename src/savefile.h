/* Numbered save files (TOURNAMN.000, SEASON.000, REPLAY.000 ...): each starts with a 26-byte Pascal string[25] description.
 * The slots are contiguous; deleting one renumbers the following ones (1008:03aa / 047f). */
#ifndef SAVEFILE_H
#define SAVEFILE_H
#include <stdio.h>

#define SAVE_DESC 26

int   slot_count(const char *base);
int   slot_read_desc(const char *base, int idx, char *desc);        /* 0-based; desc >= 27 bytes */
void  slot_write_desc(FILE *f, const char *desc);               /* Pascal string[25] */
FILE *slot_open(const char *base, int idx, const char *mode);
void  slot_delete(const char *base, int idx);

/* dialogs: return the chosen slot (0-based; == slot_count() means "(EMPTY)" slot when saving), -1 cancel */
int   dlg_slots(const char *title, const char *delete_title, const char *base, int saving);
int   dlg_description(char *desc, int maxlen);                      /* "ENTER DESCRIPTION" -> 1 ok */
#endif
