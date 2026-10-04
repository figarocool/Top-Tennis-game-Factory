#include <stdio.h>
#include <string.h>
#include "../src/pak.h"
#include "../src/gfx.h"
#include "../src/dsimg.h"
int main(int argc, char **argv)
{
    if (pak_open("orig/TENNIS.DAT")) return 1;
    if (ds_load("orig/TENNIS.EXE")) return 2;
    long total = 0; int nspr = 0, nbad = 0;
    for (int i = 0; i < pak_count(); i++) {
        const char *n = pak_name(i);
        if (strstr(n, ".CBE")) {
            Image *im = img_load_cbe(n);
            if (!im) { nbad++; continue; }
            for (int k = 0; k < im->w * im->h; k++) total += im->mask[k] ? 1 + im->px[k] : 0;
            nspr++; img_free(im);
        } else if (strstr(n, ".PBM")) {
            Image *im = img_load_pbm(n);
            if (!im) { nbad++; continue; }
            img_free(im);
        } else if (strstr(n, ".FNT")) {
            Font *f = font_load(n);
            printf("%s first=%d count=%d h=%d adv=%d\n", n, f->first, f->count, f->height, f->advance);
            font_free(f);
        }
    }
    printf("sprites=%d checksum=%ld bad=%d\n", nspr, total, nbad);
    printf("ds_size=%zu  anim frames[1..3]=%d %d %d  names: %s\n", ds_size, ds_u8(0x2f3+1), ds_u8(0x2f3+2), ds_u8(0x2f3+3), ds_cstr(0x0f16));
    return 0;
}
