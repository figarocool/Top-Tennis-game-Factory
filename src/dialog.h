/* Modal dialogs (the original's window / button / edit / list widgets of segment 1010).
 * Geometry: a window has rows*9+27 pixels of height and wchars*8+2 of width; buttons are (len+2)*8+1 by 17. */
#ifndef DIALOG_H
#define DIALOG_H
#include <stdint.h>

#define DLG_MAX_CTL 16
#define DLG_MAX_LIST 400

typedef enum { CT_LABEL, CT_EDIT, CT_BUTTON, CT_LIST } CtlType;

typedef struct {
    CtlType type;
    int  x, y;                 /* top-left in screen pixels */
    char text[96];             /* label / button caption ('@' marks the hot key) / */
    char hotkey;               /* uppercase hot key letter or 0 */
    int  hot_idx;
    int  w, h;
    int  result;               /* button: value returned by dialog_run() (0 = cancel) */
    /* edit */
    char *buf; int maxlen, visible, cursor, first;
    /* list */
    char (*items)[64]; int nitems, sel, top, rows;
} Ctl;

typedef struct {
    int  x, y, rows, wchars;
    char title[96];
    Ctl  ctl[DLG_MAX_CTL];
    int  n, focus;
} Dialog;

void dialog_init(Dialog *d, const char *title, int rows, int wchars, int y, int x);
Ctl *dialog_add_label(Dialog *d, const char *text, int y, int x);
Ctl *dialog_add_button(Dialog *d, const char *caption, int result, int y, int x);
Ctl *dialog_add_edit(Dialog *d, char *buf, int maxlen, int visible, int y, int x);
Ctl *dialog_add_list(Dialog *d, char (*items)[64], int nitems, int rows, int visible_chars, int y, int x);
int  dialog_run(Dialog *d);              /* returns the result of the activated button (0 = cancel/ESC) */

/* helpers shared with other screens */
void ui_bevel(int c_bottom_right, int c_top_left, int thick, int bottom, int right, int top, int left);   /* FUN_1010_01ad */
void ui_fill(int color, int left, int top, int right, int bottom);                                          /* inclusive rect */
int  key_ascii(int scancode);            /* scancode + shift state -> ASCII (0 if none) */
#endif
