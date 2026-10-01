#include "dsimg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t *ds_data;
size_t   ds_size;
static uint16_t ds_rel[32768];            /* DS offset (even, < 64K) -> target offset, 0 = none */

static unsigned rd16(const uint8_t *p) { return p[0] | p[1] << 8; }

/* Find the NE data segment: the segment with the DATA flag (bit0) that is not the auto-data stack stub. */
int ds_load(const char *exe_path)
{
    FILE *f = fopen(exe_path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    uint8_t *buf = malloc(len);
    fseek(f, 0, SEEK_SET);
    if (fread(buf, 1, len, f) != (size_t)len) { fclose(f); free(buf); return -1; }
    fclose(f);

    unsigned ne = rd16(buf + 0x3c) | rd16(buf + 0x3e) << 16;
    if (buf[ne] != 'N' || buf[ne + 1] != 'E') { free(buf); return -1; }
    unsigned nseg  = rd16(buf + ne + 0x1c);
    unsigned segtb = ne + rd16(buf + ne + 0x22);
    unsigned shift = rd16(buf + ne + 0x32);
    unsigned autods = rd16(buf + ne + 0x0e);           /* 1-based index of the automatic data segment */
    if (autods < 1 || autods > nseg) { free(buf); return -1; }
    const uint8_t *s = buf + segtb + (autods - 1) * 8;
    size_t off = (size_t)rd16(s) << shift, sz = rd16(s + 2);
    /* relocation records follow the segment data: u16 count, then 8-byte records */
    if (off + sz + 2 <= (size_t)len) {
        unsigned nrel = rd16(buf + off + sz);
        for (unsigned i = 0; i < nrel && off + sz + 2 + (i + 1) * 8 <= (size_t)len; i++) {
            const uint8_t *r = buf + off + sz + 2 + i * 8;
            unsigned atype = r[0], rtype = r[1] & 3, o = rd16(r + 2), t1 = rd16(r + 4), t2 = rd16(r + 6);
            if (rtype == 0 && t1 == autods && (atype == 3 || atype == 5) && o < 65536) ds_rel[o >> 1] = (uint16_t)t2;
        }
    }
    ds_data = malloc(65536);
    memset(ds_data, 0, 65536);
    memcpy(ds_data, buf + off, sz);
    ds_size = sz;
    free(buf);
    return 0;
}


unsigned ds_ptr_target(unsigned off) { return off < 65536 ? ds_rel[off >> 1] : 0; }

const char *ds_key_name(int sc)
{
    unsigned t = ds_ptr_target(0x466a + (sc & 0xff) * 4);
    return t ? ds_cstr(t) : "";
}
