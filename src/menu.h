/* Menu framework (original: the Turbo-Vision-like object library of segment 1010, menus built in 1000:a84d).
 * A menu is a full-screen background picture with a column of items: commands, sub-menus and choice items
 * ("NUMBER ... SINGLES/NUMBER ... DOUBLES": Enter cycles). Items are 14 px apart; text starts 8 px right of
 * the item's left edge, drawn outlined with the DIALOG1 font. '@' marks the hot-key letter. */
#ifndef MENU_H
#define MENU_H
#include <stdint.h>
#include "gfx.h"

#define MENU_MAX_ITEMS 24
#define MENU_MAX_CHOICES 14

typedef struct Menu Menu;
typedef struct MenuItem MenuItem;

/* result codes of an action: what the menu loop does next */
enum { MR_STAY = 0, MR_CLOSE = 1, MR_QUIT_ALL = 2 };

struct MenuItem {
    int  nchoices;                         /* 0 = command / sub-menu */
    char text[MENU_MAX_CHOICES][48];       /* display strings with '@' removed */
    char hot[MENU_MAX_CHOICES];            /* hot-key letter per string (0 = none) */
    int  hot_idx[MENU_MAX_CHOICES];        /* index of the hot-key letter inside the string */
    int  sel;                              /* current choice */
    int  (*action)(Menu *m, MenuItem *it); /* command action */
    Menu *sub;                             /* sub-menu to open (command items) */
    int  top, left, right;                 /* rectangle (inclusive) */
    int  id;                               /* user tag */
};

struct Menu {
    const char *bg_file;                   /* DATA\BACKGND\MENUxxxx.PBM */
    const char *pal_file;                  /* DATA\PAL\... */
    int  x, y;                             /* menu origin: first item at (x+1, y+1) */
    int  n;
    MenuItem items[MENU_MAX_ITEMS];
    int  sel;                              /* highlighted item (0-based) */
    int  next_top;                         /* y of the next item to add */
    Image *bg;
    Palette pal;
};

void menu_init(Menu *m, const char *bg_file, const char *pal_file, int y, int x);
MenuItem *menu_add_command(Menu *m, const char *text, int (*action)(Menu *, MenuItem *), Menu *sub);
MenuItem *menu_add_choice(Menu *m, const char *slash_separated, int selected);
int  menu_run(Menu *m);                    /* returns when closed (ESC / cancel item) */
void menu_draw(Menu *m);
void menu_run_once_draw(Menu *m);          /* load assets, set the palette and draw the menu once (used under dialogs) */                   /* background + items into vpage */
int  menu_close_action(Menu *m, MenuItem *it);       /* the CANCEL / MAIN MENU item (FUN_1010_34ae) */

/* mouse cursor (MOUSE.PBM) */
extern int ui_dirty;                     /* set by actions that replaced the screen: the menu fades back in */
void cursor_init(void);
void cursor_draw(void);
#endif
