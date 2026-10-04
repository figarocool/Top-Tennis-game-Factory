#include "dialog.h"
#include "video.h"
#include "text.h"
#include "dsimg.h"
#include "platform.h"
#include "menu.h"
#include <SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

enum { C_FILL = 3, C_LIGHT = 11, C_DARK = 12, C_BLACK = 0, C_WHITE = 15, C_YELLOW = 14, C_BLUE = 1, C_RED = 4 };

void ui_fill(int color, int left, int top, int right, int bottom)
{
    fill_rect(vpage, VW, VH, left, top, right - left + 1, bottom - top + 1, (uint8_t)color);
}

static void hline(int c, int y, int x1, int x2) { if (x1 > x2) { int t = x1; x1 = x2; x2 = t; } fill_rect(vpage, VW, VH, x1, y, x2 - x1 + 1, 1, (uint8_t)c); }
static void vline(int c, int x, int y1, int y2) { if (y1 > y2) { int t = y1; y1 = y2; y2 = t; } fill_rect(vpage, VW, VH, x, y1, 1, y2 - y1 + 1, (uint8_t)c); }

/* FUN_1010_01ad: `thick` nested rectangles; bottom and right edges in c_br, top and left edges in c_tl */
void ui_bevel(int c_br, int c_tl, int thick, int bottom, int right, int top, int left)
{
    for (int i = 0; i < thick; i++) {
        hline(c_br, bottom - i, right - i, i + left);
        vline(c_br, right - i, bottom - i, i + top);
        hline(c_tl, i + top, right - i, i + left);
        vline(c_tl, i + left, bottom - i, i + top);
    }
}

int key_ascii(int sc)
{
    if (sc <= 0 || sc >= 0x80) return 0;
    int shift = keys[0x2a] || keys[0x36] || (SDL_GetModState() & KMOD_CAPS);
    int a = ds_u8((shift ? 0x4cd2 : 0x4c52) + sc);
    if ((SDL_GetModState() & KMOD_CAPS) && !(keys[0x2a] || keys[0x36]) && !isalpha(a)) a = ds_u8(0x4c52 + sc);
    return a;
}

/* ---------------------------------------------------------------- construction */

static void split_hot(Ctl *c, const char *s)
{
    c->hotkey = 0; c->hot_idx = -1;
    int o = 0;
    for (; *s && o < 94; s++) {
        if (*s == '@' && s[1]) { c->hot_idx = o; c->hotkey = (char)toupper((unsigned char)s[1]); continue; }
        c->text[o++] = *s;
    }
    c->text[o] = 0;
}

void dialog_init(Dialog *d, const char *title, int rows, int wchars, int y, int x)
{
    memset(d, 0, sizeof *d);
    snprintf(d->title, sizeof d->title, "%s", title);
    d->rows = rows; d->wchars = wchars; d->y = y; d->x = x;
}

static Ctl *new_ctl(Dialog *d, CtlType t)
{
    if (d->n >= DLG_MAX_CTL) return NULL;
    Ctl *c = &d->ctl[d->n++];
    memset(c, 0, sizeof *c);
    c->type = t;
    return c;
}

Ctl *dialog_add_label(Dialog *d, const char *text, int y, int x)
{
    Ctl *c = new_ctl(d, CT_LABEL);
    snprintf(c->text, sizeof c->text, "%s", text);
    c->x = x; c->y = y;
    return c;
}

Ctl *dialog_add_button(Dialog *d, const char *caption, int result, int y, int x)
{
    Ctl *c = new_ctl(d, CT_BUTTON);
    split_hot(c, caption);
    c->x = x; c->y = y;
    c->w = ((int)strlen(c->text) + 2) * 8 + 1;
    c->h = 17;
    c->result = result;
    return c;
}

Ctl *dialog_add_edit(Dialog *d, char *buf, int maxlen, int visible, int y, int x)
{
    Ctl *c = new_ctl(d, CT_EDIT);
    c->buf = buf; c->maxlen = maxlen; c->visible = visible;
    c->x = x; c->y = y; c->w = visible * 8; c->h = 9;
    c->cursor = (int)strlen(buf); c->first = 0;
    return c;
}

Ctl *dialog_add_list(Dialog *d, char (*items)[64], int nitems, int rows, int visible_chars, int y, int x)
{
    Ctl *c = new_ctl(d, CT_LIST);
    c->items = items; c->nitems = nitems; c->rows = rows;
    c->x = x; c->y = y; c->w = visible_chars * 8; c->h = rows * 9 + 2;
    c->sel = 0; c->top = 0;
    return c;
}

/* ---------------------------------------------------------------- drawing */

static void draw_window(const Dialog *d)
{
    int left = d->x, top = d->y, right = d->x + 2 + d->wchars * 8, bottom = (d->rows + 3) * 9 + d->y;
    ui_fill(C_FILL, left, top, right, bottom);
    ui_bevel(C_DARK, C_LIGHT, 1, bottom, right, top, left);
    ui_bevel(C_LIGHT, C_DARK, 1, bottom - 8, right - 8, top + 11, left + 8);
    text_at(d->title, C_BLACK, DST_PAGE, top + 3, left + 9);
}

static void draw_button(const Ctl *c, int focus)
{
    int left = c->x, top = c->y, right = c->x + c->w, bottom = c->y + c->h;
    ui_fill(C_FILL, left, top, right, bottom);
    ui_bevel(C_DARK, C_LIGHT, 1, bottom, right, top, left);
    text_at(c->text, focus ? C_WHITE : C_BLACK, DST_PAGE, top + 5, left + 9);
    if (c->hotkey) {
        char ch[2] = { c->text[c->hot_idx], 0 };
        text_at(ch, C_YELLOW, DST_PAGE, top + 5, left + 1 + (c->hot_idx + 1) * 8);
    }
}

static void draw_edit(const Ctl *c, int focus)
{
    ui_fill(C_BLUE, c->x, c->y, c->x + c->w - 1, c->y + c->h - 1);
    for (int i = 0; i < c->visible; i++) {
        int k = c->first + i;
        if (k < (int)strlen(c->buf)) { char ch[2] = { c->buf[k], 0 }; text_at(ch, C_YELLOW, DST_PAGE, c->y + 1, c->x + i * 8); }
    }
    if (focus) {
        int cx = c->x + (c->cursor - c->first) * 8;
        ui_fill(C_RED, cx, c->y, cx + 7, c->y + 8);
        if (c->cursor < (int)strlen(c->buf)) { char ch[2] = { c->buf[c->cursor], 0 }; text_at(ch, C_YELLOW, DST_PAGE, c->y + 1, cx); }
    }
}

/* FUN_1010_241b: grey list box with a scroll column; the selected row is blue with white text */
static void draw_list(const Ctl *c, int focus)
{
    int left = c->x, top = c->y, right = c->x + c->w, bottom = c->y + c->h - 1;
    ui_fill(7, left, top, right, bottom);
    ui_bevel(C_DARK, C_DARK, 1, bottom, right + 1, top, left);           /* FUN_1010_0c9a: 4 lines in colour 12 */
    text_glyph(0x7b, 15, top + 2, right - 7);
    text_glyph(0x7d, 15, bottom - 8, right - 7);
    vline(C_DARK, right - 8, bottom, top);
    hline(C_DARK, top + 10, right - 1, right - 8);
    hline(C_DARK, bottom - 10, right - 1, right - 8);
    for (int r = 0; r < c->rows; r++) {
        int k = c->top + r;
        if (k >= c->nitems) break;
        int selected = k == c->sel;
        if (selected) ui_fill(focus ? 1 : 7, left + 1, top + 1 + r * 9, right - 10, top + 9 + r * 9);
        text_at(c->items[k], selected ? 15 : 0, DST_PAGE, top + 2 + r * 9, left + 1);
    }
}

static void draw_dialog(const Dialog *d)
{
    draw_window(d);
    for (int i = 0; i < d->n; i++) {
        const Ctl *c = &d->ctl[i];
        int focus = i == d->focus;
        switch (c->type) {
        case CT_LABEL: text_at(c->text, C_BLACK, DST_PAGE, c->y, c->x); break;
        case CT_BUTTON: draw_button(c, focus); break;
        case CT_EDIT: draw_edit(c, focus); break;
        case CT_LIST: draw_list(c, focus); break;
        }
    }
}

/* ---------------------------------------------------------------- run */

static int focusable(const Ctl *c) { return c->type != CT_LABEL; }

static void focus_next(Dialog *d, int dir)
{
    for (int k = 0; k < d->n; k++) {
        d->focus = (d->focus + dir + d->n) % d->n;
        if (focusable(&d->ctl[d->focus])) return;
    }
}

static int default_button(const Dialog *d)
{
    for (int i = 0; i < d->n; i++) if (d->ctl[i].type == CT_BUTTON) return d->ctl[i].result;
    return 0;
}

#if defined(__vita__) || defined(__PSP__) || defined(TT_TOUCHKB)
/* no keyboard: Up/Down cycle the letter before the cursor, Right starts the next letter, Left erases */
static void vita_letter(Ctl *c, int sc)
{
    static const char set[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-";
    static int pending;
    int len = (int)strlen(c->buf);
    if (sc == 0xc8 || sc == 0xd0) {
        int n = (int)sizeof set - 1;
        if (!pending || c->cursor == 0) {
            if (len >= c->maxlen) return;
            memmove(c->buf + c->cursor + 1, c->buf + c->cursor, len - c->cursor + 1);
            c->buf[c->cursor++] = sc == 0xc8 ? 'A' : 'Z';
            pending = 1;
        } else {
            char *ch = &c->buf[c->cursor - 1];
            int i = (int)(strchr(set, *ch) ? strchr(set, *ch) - set : 0);
            i = (i + (sc == 0xc8 ? 1 : n - 1)) % n;
            *ch = set[i];
        }
    } else if (sc == 0xcd) pending = 0;
    else if (sc == 0xcb) {
        pending = 0;
        if (c->cursor > 0) { memmove(c->buf + c->cursor - 1, c->buf + c->cursor, len - c->cursor + 1); c->cursor--; }
    }
    if (c->cursor < c->first) c->first = c->cursor;
    if (c->cursor >= c->first + c->visible) c->first = c->cursor - c->visible + 1;
}
#endif

#if defined(__vita__) || defined(__PSP__) || defined(TT_TOUCHKB)
/* On-screen keyboard for the touch screen: 14 x 3 keys under the dialog. */
enum { VK_COLS = 14, VK_ROWS = 3, VK_W = 22, VK_H = 18, VK_X = 6, VK_Y = 145 };
static const char vk_keys[VK_COLS * VK_ROWS + 1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-\x04\x03\x01\x02";   /* 4 space, 3 backspace, 1 OK, 2 cancel */

static void vk_draw(void)
{
    for (int r = 0; r < VK_ROWS; r++)
        for (int c = 0; c < VK_COLS; c++) {
            int x = VK_X + c * VK_W, y = VK_Y + r * VK_H;
            char k = vk_keys[r * VK_COLS + c];
            ui_fill(3, x, y, x + VK_W - 1, y + VK_H - 1);
            ui_bevel(C_DARK, C_LIGHT, 1, y + VK_H - 1, x + VK_W - 1, y, x);
            char lab[3] = { k, 0, 0 };
            if (k == 1) { lab[0] = 'O'; lab[1] = 'K'; }
            else if (k == 2) lab[0] = 'X';
            else if (k == 3) lab[0] = '<';
            else if (k == 4) lab[0] = '_';
            text_at(lab, k < 5 ? 14 : 15, DST_PAGE, y + 5, x + (lab[1] ? 3 : 8));
        }
}

/* key under the game-pixel position, or 0 */
static int vk_hit(int gx, int gy)
{
    int c = (gx - VK_X) / VK_W, r = (gy - VK_Y) / VK_H;
    if (gx < VK_X || gy < VK_Y || c >= VK_COLS || r >= VK_ROWS) return 0;
    return vk_keys[r * VK_COLS + c];
}
#endif

static void edit_key(Ctl *c, int sc, int ch)
{
#if defined(__vita__) || defined(__PSP__) || defined(TT_TOUCHKB)
    if (sc == 0x1f) sc = 0xcb;                              /* the pad's square button (mapped to S) erases, like left */
    if (sc == 0xc8 || sc == 0xd0 || sc == 0xcb || sc == 0xcd) { vita_letter(c, sc); return; }
#endif
    int len = (int)strlen(c->buf);
    if (sc == 0xcb) { if (c->cursor > 0) c->cursor--; }
    else if (sc == 0xcd) { if (c->cursor < len) c->cursor++; }
    else if (sc == 0xc7) c->cursor = 0;
    else if (sc == 0xcf) c->cursor = len;
    else if (sc == 0x0e) { if (c->cursor > 0) { memmove(c->buf + c->cursor - 1, c->buf + c->cursor, len - c->cursor + 1); c->cursor--; } }
    else if (sc == 0xd3) { if (c->cursor < len) memmove(c->buf + c->cursor, c->buf + c->cursor + 1, len - c->cursor); }
    else if (ch >= 0x20 && ch < 0x7f && len < c->maxlen) {
        memmove(c->buf + c->cursor + 1, c->buf + c->cursor, len - c->cursor + 1);
        c->buf[c->cursor++] = (char)ch;
    }
    if (c->cursor < c->first) c->first = c->cursor;
    if (c->cursor >= c->first + c->visible) c->first = c->cursor - c->visible + 1;
}

int (*dialog_tick)(void);        /* optional: polled every frame by dialog_run; a non-zero value closes the dialog with it */

int dialog_run(Dialog *d)
{
    static uint8_t snap[VW * VH];
    memcpy(snap, vpage, sizeof snap);
    d->focus = 0;
    for (int i = 0; i < d->n; i++) if (d->ctl[i].type == CT_EDIT || d->ctl[i].type == CT_LIST) { d->focus = i; break; }
    int result = -1;
    int previous_pointer_mode = plat_pointer_mode(1);
    key_flush();
    while (result < 0 && !quit_requested) {
        memcpy(vpage, snap, sizeof snap);
        draw_dialog(d);
#if defined(__vita__) || defined(__PSP__) || defined(TT_TOUCHKB)
        static int kb_hidden;
        int kb = d->ctl[d->focus].type == CT_EDIT && !kb_hidden;
        if (kb) vk_draw();
#endif
        cursor_draw();
        video_wait_vsync();
        int sc = key_pressed_scancode();
        if (sc) {
            Ctl *f = &d->ctl[d->focus];
            int shift = keys[0x2a] || keys[0x36];
#if defined(__vita__) || defined(__PSP__) || defined(TT_TOUCHKB)
            if (sc == 0x3d && d->ctl[d->focus].type == CT_EDIT) { kb_hidden = !kb_hidden; sc = 0; }       /* triangle: show / hide the keyboard */
            if (sc == 0x1f && d->ctl[d->focus].type != CT_EDIT)       /* square = "no" in yes/no questions */
                for (int i = 0; i < d->n; i++)
                    if (d->ctl[i].type == CT_BUTTON && d->ctl[i].hotkey == 'N') { result = d->ctl[i].result; sc = 0; break; }
#endif
            if (sc == 0) { /* handled above */ }
            else if (sc == 0x01) result = 0;
            else if (sc == 0x0f) focus_next(d, shift ? -1 : 1);
            else if (sc == 0x1c || sc == 0x9c || sc == 0x39) {
                if (f->type == CT_BUTTON) result = f->result;
                else if (f->type == CT_LIST && f->sel < f->nitems) result = default_button(d);
                else if (f->type == CT_EDIT && sc != 0x39) result = default_button(d);
                else if (f->type == CT_EDIT) edit_key(f, sc, ' ');
            } else if (f->type == CT_EDIT) edit_key(f, sc, key_ascii(sc));
            else if (f->type == CT_LIST) {
                if (sc == 0xc8 && f->sel > 0) f->sel--;
                else if (sc == 0xd0 && f->sel + 1 < f->nitems) f->sel++;
                else if (sc == 0xc7) f->sel = 0;
                else if (sc == 0xcf) f->sel = f->nitems ? f->nitems - 1 : 0;
                else if (sc == 0xc9) f->sel = f->sel > f->rows ? f->sel - f->rows : 0;
                else if (sc == 0xd1) f->sel = f->sel + f->rows < f->nitems ? f->sel + f->rows : f->nitems - 1;
                if (f->sel < f->top) f->top = f->sel;
                if (f->sel >= f->top + f->rows) f->top = f->sel - f->rows + 1;
            }
            /* hot keys of buttons (Alt not required) */
            if (f->type != CT_EDIT && result < 0) {
                int a = key_ascii(sc);
                if (a) for (int i = 0; i < d->n; i++)
                    if (d->ctl[i].type == CT_BUTTON && d->ctl[i].hotkey && d->ctl[i].hotkey == toupper(a)) { result = d->ctl[i].result; break; }
            }
        }
        if (dialog_tick && result < 0) { int t = dialog_tick(); if (t) result = t; }
        /* mouse: click on a control */
        int mx = 0, my = 0;
        static int prev;
        {
            int down = plat_mouse(&mx, &my);
            int click = down && !prev;
#if defined(__vita__) || defined(__PSP__) || defined(TT_TOUCHKB)
            if (click && d->ctl[d->focus].type == CT_EDIT) {
                Ctl *f = &d->ctl[d->focus];
                int inside_edit = mx >= f->x && mx <= f->x + f->w && my >= f->y && my <= f->y + f->h;
                if (kb && my >= VK_Y) {                    /* a key of the touch keyboard */
                    int k = vk_hit(mx, my);
                    if (k == 1) result = default_button(d);
                    else if (k == 2) result = 0;
                    else if (k == 3) edit_key(f, 0x0e, 0);
                    else if (k == 4) edit_key(f, 0x39, ' ');
                    else if (k) edit_key(f, 0, k);
                    click = 0;
                } else if (inside_edit) { kb_hidden = 0; click = 0; }        /* tap the field: keyboard back */
                else if (kb) { kb_hidden = 1; click = 0; }                      /* tap elsewhere: close it */
            }
#endif
            if (click)
                for (int i = 0; i < d->n; i++) {
                    Ctl *c = &d->ctl[i];
                    if (!focusable(c) || mx < c->x || mx > c->x + c->w || my < c->y || my > c->y + c->h) continue;
                    d->focus = i;
                    if (c->type == CT_BUTTON) result = c->result;
                    else if (c->type == CT_LIST) { int k = c->top + (my - c->y - 1) / 9; if (k < c->nitems) c->sel = k; }
                    break;
                }
            prev = down;
        }
    }
    memcpy(vpage, snap, sizeof snap);
    plat_pointer_mode(previous_pointer_mode);
    return quit_requested ? 0 : result;
}
