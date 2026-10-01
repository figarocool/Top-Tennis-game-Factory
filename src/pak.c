#include "pak.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { char name[57]; uint32_t size, offset; } Entry;

static FILE  *fp;
static Entry *dir;
static int    count;

static void norm(char *dst, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i + 1 < n && src[i]; i++)
        dst[i] = src[i] == '/' ? '\\' : (char)toupper((unsigned char)src[i]);
    dst[i] = 0;
}

int pak_open(const char *path)
{
    uint8_t hdr[6];
    fp = fopen(path, "rb");
    if (!fp) return -1;
    if (fread(hdr, 1, 6, fp) != 6) return -1;
    count = hdr[0] | hdr[1] << 8;
    uint32_t dirpos = hdr[2] | hdr[3] << 8 | hdr[4] << 16 | (uint32_t)hdr[5] << 24;
    uint8_t *raw = malloc((size_t)count * 64);
    if (fseek(fp, dirpos, SEEK_SET) || fread(raw, 64, count, fp) != (size_t)count) return -1;
    dir = calloc(count, sizeof *dir);
    for (int i = 0; i < count; i++) {
        const uint8_t *e = raw + i * 64;
        char tmp[57];
        memcpy(tmp, e, 56); tmp[56] = 0;
        norm(dir[i].name, tmp, sizeof dir[i].name);
        dir[i].size   = e[56] | e[57] << 8 | e[58] << 16 | (uint32_t)e[59] << 24;
        dir[i].offset = e[60] | e[61] << 8 | e[62] << 16 | (uint32_t)e[63] << 24;
    }
    free(raw);
    return 0;
}

void pak_close(void)
{
    if (fp) fclose(fp);
    free(dir);
    fp = NULL; dir = NULL; count = 0;
}

int pak_count(void) { return count; }
const char *pak_name(int idx) { return dir[idx].name; }

uint8_t *pak_load(const char *name, size_t *size)
{
    char key[64];
    norm(key, name, sizeof key);
    for (int i = 0; i < count; i++) {
        if (strcmp(dir[i].name, key)) continue;
        uint8_t *buf = malloc(dir[i].size ? dir[i].size : 1);
        if (fseek(fp, dir[i].offset, SEEK_SET) || fread(buf, 1, dir[i].size, fp) != dir[i].size) {
            free(buf);
            return NULL;
        }
        if (size) *size = dir[i].size;
        return buf;
    }
    return NULL;
}
