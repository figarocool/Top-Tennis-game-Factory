#include "menu.h"
#include "video.h"
#include "text.h"
#include "platform.h"
#include "ui.h"
#include <SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { COL_NORMAL = 11, COL_SELECTED = 14, COL_HOT = 10, COL_OUTLINE = 0 };
enum { ITEM_H = 14 };

void menu_init(Menu *m, const char *bg_file, const char *pal_file, int y, int x)
{
    memset(m, 0, sizeof *m);
    m->bg_file = bg_file;
    m->pal_file = pal_file;
    m->x = x; m->y = y;
    m->next_top = y + 1;
}

/* split "A@B/..." into display text (without '@') and hot-key info */
static void parse_text(MenuItem *it, int k, const char *s)
{
    char *d = it->text[k];
    it->hot[k] = 0; it->hot_idx[k] = -1;
    for (int i = 0; *s && i < 47; s++) {
        if (*s == '@' && s[1]) { it->hot_idx[k] = i; it->hot[k] = (char)toupper((unsigned char)s[1]); continue; }
        d[i++] = *s;
        d[i] = 0;
    }
}

static MenuItem *new_item(Menu *m, int len_chars)
{
    if (m->n >= MENU_MAX_ITEMS) return NULL;
    MenuItem *it = &m->items[m->n++];
    memset(it, 0, sizeof *it);
    it->top = m->next_top;
    it->left = m->x + 1;
    it->right = it->left + len_chars * 8 - 1;
    m->next_top += ITEM_H;
    return it;
}

MenuItem *menu_add_command(Menu *m, const char *text, int (*action)(Menu *, MenuItem *), Menu *sub)
{
    MenuItem *it = new_item(m, (int)strlen(text));
    if (!it) return NULL;
    parse_text(it, 0, text);
    it->nchoices = 0;
    it->action = action;
    it->sub = sub;
    return it;
}

MenuItem *menu_add_choice(Menu *m, const char *list, int selected)
{
    char buf[512];
    snprintf(buf, sizeof buf, "%s", list);
    MenuItem *it = new_item(m, 0);
    if (!it) return NULL;
    int k = 0, maxlen = 0;
    for (char *tok = strtok(buf, "/"); tok && k < MENU_MAX_CHOICES; tok = strtok(NULL, "/"), k++) {
        parse_text(it, k, tok);
        int l = (int)strlen(it->text[k]);
        if (l > maxlen) maxlen = l;
    }
    it->nchoices = k;
    it->sel = selected;
    it->right = it->left + (maxlen + 1) * 8 - 1;
    return it;
}

int menu_close_action(Menu *m, MenuItem *it) { (void)m; (void)it; return MR_CLOSE; }

/* ---------------------------------------------------------------- drawing */

static void draw_item(Menu *m, int idx, int selected)
{
    MenuItem *it = &m->items[idx];
    int k = it->nchoices ? it->sel : 0;
    const char *s = it->text[k];
    int x = it->left + 8, y = it->top;
    /* restore the background under the item (the original redraws from the saved picture) */
    for (int j = it->top; j < it->top + ITEM_H && j < VH; j++)
        for (int i = it->left; i <= it->right + 8 && i < VW; i++)
            if (m->bg && j < m->bg->h && i < m->bg->w) vpage[j * VW + i] = m->bg->px[j * m->bg->w + i];
    font_select(5);
    text_outlined(s, COL_OUTLINE, selected ? COL_SELECTED : COL_NORMAL, DST_PAGE, y, x);
    if (!selected && it->hot[k]) {
        char c[2] = { s[it->hot_idx[k]], 0 };
        text_at(c, COL_HOT, DST_PAGE, y, x + it->hot_idx[k] * 8);
    }
}

void menu_draw(Menu *m)
{
    if (m->bg) for (int y = 0; y < m->bg->h && y < VH; y++) memcpy(vpage + y * VW, m->bg->px + y * m->bg->w, m->bg->w);
    for (int i = 0; i < m->n; i++) draw_item(m, i, i == m->sel);
}

/* ---------------------------------------------------------------- mouse cursor */

static Image *cursor_img;
int ui_dirty;
static int mouse_x = 160, mouse_y = 100, mouse_btn, mouse_btn_prev;

void cursor_init(void)
{
    if (!cursor_img) cursor_img = img_load_pbm("DATA\\BACKGND\\MOUSE.PBM");
    if (cursor_img && !cursor_img->mask) {            /* colour 0 is transparent */
        cursor_img->mask = malloc((size_t)cursor_img->w * cursor_img->h);
        for (int i = 0; i < cursor_img->w * cursor_img->h; i++) cursor_img->mask[i] = cursor_img->px[i] != 0;
    }
}

void cursor_draw(void)
{
    plat_mouse(&mouse_x, &mouse_y);
    if (cursor_img) blit(vpage, VW, VH, cursor_img, mouse_x + scroll_x, mouse_y + scroll_y);
}

/* ---------------------------------------------------------------- run */

static int hit(const MenuItem *it, int x, int y) { return y >= it->top && y < it->top + ITEM_H && x >= it->left && x <= it->right + 8; }

static int activate(Menu *m, int idx)
{
    MenuItem *it = &m->items[idx];
    m->sel = idx;
    menu_draw(m);                      /* show the selection before the action takes over the screen */
    if (it->nchoices) {
        it->sel = (it->sel + 1) % it->nchoices;
        return MR_STAY;
    }
    if (it->sub) { int r = menu_run(it->sub); ui_dirty = 1; return r == MR_QUIT_ALL ? MR_QUIT_ALL : MR_STAY; }
    if (it->action) return it->action(m, it);
    return MR_STAY;
}

void menu_run_once_draw(Menu *m)
{
    if (!m->bg) { m->bg = img_load_pbm(m->bg_file); pal_load(m->pal_file, &m->pal); }
    video_set_palette(&m->pal);
    video_set_split(200);
    video_set_scroll(0, 0);
    menu_draw(m);
}

/* a menu opens like the original: the background fades in on its own, then the items are drawn */
static void menu_enter(Menu *m)
{
    video_black();
    if (m->bg) for (int y = 0; y < m->bg->h && y < VH; y++) memcpy(vpage + y * VW, m->bg->px + y * m->bg->w, m->bg->w);
    video_set_split(200);
    video_set_scroll(0, 0);
    video_fade_in(&m->pal, 2);
    key_flush();
}

int menu_run(Menu *m)
{
    if (!m->bg) {
        m->bg = img_load_pbm(m->bg_file);
        pal_load(m->pal_file, &m->pal);
    }
    int result = MR_STAY;
    int enter = 1;
    while (!quit_requested) {
        if (enter) { enter = 0; menu_enter(m); }
        ui_apply_options();
        menu_draw(m);
        cursor_draw();
        video_wait_vsync();
        result = MR_STAY;
        int k = key_pressed_scancode();
        if (k == 0xc8 || k == 0x48) m->sel = (m->sel + m->n - 1) % m->n;
        else if (k == 0xd0 || k == 0x50) m->sel = (m->sel + 1) % m->n;
        else if (k == 0x1c || k == 0x39 || k == 0x9c) result = activate(m, m->sel);
        else if (k == 0x01) result = MR_CLOSE;
        else if (k) {
            static const char *row1 = "QWERTYUIOP", *row2 = "ASDFGHJKL", *row3 = "ZXCVBNM";
            char c = 0;
            if (k >= 0x10 && k <= 0x19) c = row1[k - 0x10];
            else if (k >= 0x1e && k <= 0x26) c = row2[k - 0x1e];
            else if (k >= 0x2c && k <= 0x32) c = row3[k - 0x2c];
            else if (k >= 0x02 && k <= 0x0a) c = (char)('1' + k - 2);
            else if (k == 0x0b) c = '0';
            for (int i = 0; c && i < m->n; i++) {
                MenuItem *it = &m->items[i];
                int kk = it->nchoices ? it->sel : 0;
                if (it->hot[kk] == c) { m->sel = i; result = activate(m, i); break; }
            }
        }
        mouse_btn_prev = mouse_btn;
        mouse_btn = plat_mouse(&mouse_x, &mouse_y);
        if (mouse_btn && !mouse_btn_prev)
            for (int i = 0; i < m->n; i++)
                if (hit(&m->items[i], mouse_x, mouse_y)) { m->sel = i; result = activate(m, i); break; }
        if (ui_dirty) { ui_dirty = 0; enter = 1; }
        if (result == MR_CLOSE || result == MR_QUIT_ALL) { video_fade_out(2); return result; }
    }
    return MR_QUIT_ALL;
}
