#include "dsimg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t *ds_data;
size_t   ds_size;
static uint16_t ds_rel[32768];            /* DS offset (even, < 64K) -> target offset, 0 = none */

extern const unsigned ds_image_size;
extern const uint8_t ds_image[];
extern const unsigned ds_reloc_count;
extern const uint16_t ds_reloc[][2];

/* The constant tables and strings of the program's data segment are compiled in (src/dsdata.c). */
int ds_load(const char *unused)
{
    (void)unused;
    ds_data = malloc(65536);
    memset(ds_data, 0, 65536);
    memcpy(ds_data, ds_image, ds_image_size);
    ds_size = ds_image_size;
    for (unsigned i = 0; i < ds_reloc_count; i++)
        ds_rel[ds_reloc[i][0] >> 1] = ds_reloc[i][1];
    return 0;
}

unsigned ds_ptr_target(unsigned off) { return off < 65536 ? ds_rel[off >> 1] : 0; }

const char *ds_key_name(int sc)
{
    unsigned t = ds_ptr_target(0x466a + (sc & 0xff) * 4);
    return t ? ds_cstr(t) : "";
}
