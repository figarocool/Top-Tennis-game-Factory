#include "dsimg.h"
#include "savefile.h"
#include "dialog.h"
#include "text.h"
#include "options.h"
#include "platform.h"
#include <string.h>

static void slot_path(char *p, size_t n, const char *base, int idx) { snprintf(p, n, "%s/%s.%03d", data_dir, base, idx); }

int slot_count(const char *base)
{
    int n = 0;
    for (;; n++) {
        char p[600]; slot_path(p, sizeof p, base, n);
        FILE *f = fopen(p, "rb");
        if (!f) break;
        fclose(f);
    }
    return n;
}

FILE *slot_open(const char *base, int idx, const char *mode)
{
    char p[600]; slot_path(p, sizeof p, base, idx);
    return fopen(p, mode);
}

int slot_read_desc(const char *base, int idx, char *desc)
{
    FILE *f = slot_open(base, idx, "rb");
    if (!f) { desc[0] = 0; return 0; }
    uint8_t b[SAVE_DESC];
    memset(b, 0, sizeof b);
    size_t n = fread(b, 1, SAVE_DESC, f);
    fclose(f);
    (void)n;
    int l = b[0] > SAVE_DESC - 1 ? SAVE_DESC - 1 : b[0];       /* Pascal string[25] */
    memcpy(desc, b + 1, l);
    desc[l] = 0;
    return 1;
}

void slot_delete(const char *base, int idx)
{
    int n = slot_count(base);
    char a[600], b[600];
    slot_path(a, sizeof a, base, idx);
    remove(a);
    for (int i = idx + 1; i < n; i++) {
        slot_path(a, sizeof a, base, i);
        slot_path(b, sizeof b, base, i - 1);
        rename(a, b);
    }
}

void slot_write_desc(FILE *f, const char *desc)
{
    uint8_t b[SAVE_DESC];
    memset(b, 0, sizeof b);
    int l = (int)strlen(desc);
    if (l > SAVE_DESC - 1) l = SAVE_DESC - 1;
    b[0] = (uint8_t)l;
    memcpy(b + 1, desc, l);
    fwrite(b, 1, sizeof b, f);
}

int dlg_description(char *desc, int maxlen)
{
    Dialog d;
    char buf[40];
    snprintf(buf, sizeof buf, "%.*s", maxlen, desc);
    dialog_init(&d, ds_cstr(0x3040), 5, 0x1c, 0x60, 0x32);
    dialog_add_edit(&d, buf, maxlen, 0x18, 0x77, 0x42);
    dialog_add_button(&d, ds_cstr(0x3057), 1, 0x8a, 0x46);
    dialog_add_button(&d, ds_cstr(0x1c2c), 0, 0x8a, 0xbe);
    font_select(8);
    if (!dialog_run(&d)) return 0;
    snprintf(desc, maxlen + 1, "%s", buf);
    return 1;
}

int dlg_slots(const char *title, const char *delete_title, const char *base, int saving)
{
    for (;;) {
        static char items[DLG_MAX_LIST][64];
        int n = slot_count(base);
        if (n > DLG_MAX_LIST - 1) n = DLG_MAX_LIST - 1;
        for (int i = 0; i < n; i++) {
            char d[SAVE_DESC + 1];
            slot_read_desc(base, i, d);
            snprintf(items[i], sizeof items[i], "%s", d);
        }
        int count = n;
        if (saving) snprintf(items[count++], sizeof items[0], "%s", ds_cstr(0x2cf0));
        if (!saving && n == 0) return -1;
        Dialog d;
        dialog_init(&d, title, 15, 0x1d, 15, 0x2d);
        Ctl *l = dialog_add_list(&d, items, count, 10, 25, 0x24, 0x3e);
        (void)l;
        dialog_add_button(&d, saving ? ds_cstr(0x2fe6) : ds_cstr(0x2fbc), 1, 0x8c, 0x3c);
        dialog_add_button(&d, ds_cstr(0x2fc2), 2, 0x8c, 0x7b);
        dialog_add_button(&d, ds_cstr(0x1c2c), 0, 0x8c, 200);
        font_select(8);
        int r = dialog_run(&d);
        int sel = d.ctl[0].sel;
        if (r == 0) return -1;
        if (r == 1) return sel;
        if (r == 2 && sel < n) {
            Dialog q;
            dialog_init(&q, delete_title, 5, 0x1b, 0x60, 0x32);
            dialog_add_button(&q, ds_cstr(0x3067), 1, 0x8a, 0x46);
            dialog_add_button(&q, ds_cstr(0x306e), 0, 0x8a, 0xbe);
            font_select(8);
            if (dialog_run(&q)) slot_delete(base, sel);
        }
    }
}
