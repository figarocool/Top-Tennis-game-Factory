/* Reads compatibility data supplied by the user, never executable instructions.
 * Supports a locally prepared DAT or the NE data segment of the original EXE. */
#include "dsimg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DS_CAPACITY 65536u
#define EXE_LIMIT (4u * 1024u * 1024u)
#define SUPPORTED_DS_SIZE 19876u
#define SUPPORTED_DS_CHECK 0x3994e248u
#define SUPPORTED_RELOC_CHECK 0x265717bdu
#define FOOTER_SIZE 24u

uint8_t *ds_data;
size_t ds_size;
static uint16_t ds_rel[DS_CAPACITY / 2];
static const char *error_text = "TENNIS.EXE NOT LOADED";
static uint32_t data_check;

static unsigned u16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static uint32_t u32(const uint8_t *p) { return u16(p) | (uint32_t)u16(p + 2) << 16; }
static int fits(size_t at, size_t size, size_t n) { return at <= n && size <= n - at; }
static uint32_t hash_bytes(uint32_t h, const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

const char *ds_error(void) { return error_text; }
uint32_t ds_checksum(void) { return data_check; }

void ds_unload(void)
{
    free(ds_data);
    ds_data = NULL;
    ds_size = 0;
    data_check = 0;
    memset(ds_rel, 0, sizeof ds_rel);
}

/* Commit only a fully validated image. Hashes identify a supported layout,
 * not ownership, authenticity or a licence. */
static int install(const uint8_t *data, size_t size, const uint16_t *rel)
{
    error_text = "UNSUPPORTED ORIGINAL GAME VERSION";
    uint32_t check = hash_bytes(2166136261u, data, size), reloc_check = 2166136261u;
    unsigned count = 0;
    if (size != SUPPORTED_DS_SIZE || check != SUPPORTED_DS_CHECK) return -1;
    for (unsigned at = 0; at < DS_CAPACITY; at += 2) {
        unsigned target = rel[at / 2];
        if (!target) continue;
        if (!fits(at, 4, size) || target >= size) return -1;
        uint8_t pair[4] = { (uint8_t)at, (uint8_t)(at >> 8), (uint8_t)target, (uint8_t)(target >> 8) };
        reloc_check = hash_bytes(reloc_check, pair, sizeof pair);
        count++;
    }
    if (count != 101 || reloc_check != SUPPORTED_RELOC_CHECK) return -1;
    error_text = "NOT ENOUGH MEMORY";
    ds_data = calloc(DS_CAPACITY, 1);
    if (!ds_data) return -1;
    memcpy(ds_data, data, size);
    memcpy(ds_rel, rel, sizeof ds_rel);
    ds_size = size;
    data_check = check ^ reloc_check;
    error_text = "";
    return 0;
}

int ds_load(const char *path)
{
    uint8_t *file = NULL;
    uint16_t *rel = NULL;
    FILE *f = NULL;
    int result = -1;
    ds_unload();
    error_text = "TENNIS.EXE NOT FOUND";
    if (!path || !(f = fopen(path, "rb"))) return -1;
    error_text = "CANNOT READ TENNIS.EXE";
    if (fseek(f, 0, SEEK_END)) goto done;
    long length = ftell(f);
    if (length < 64 || (unsigned long)length > EXE_LIMIT || fseek(f, 0, SEEK_SET)) goto done;
    size_t n = (size_t)length;
    error_text = "NOT ENOUGH MEMORY";
    file = malloc(n);
    if (!file) goto done;
    error_text = "CANNOT READ TENNIS.EXE";
    if (fread(file, 1, n, f) != n) goto done;
    error_text = "INVALID TENNIS.EXE";
    if (file[0] != 'M' || file[1] != 'Z') goto done;
    size_t ne = u32(file + 0x3c);
    if (!fits(ne, 64, n) || file[ne] != 'N' || file[ne + 1] != 'E') goto done;
    unsigned count = u16(file + ne + 0x1c), index = u16(file + ne + 0x0e);
    unsigned shift = u16(file + ne + 0x32);
    size_t table = ne + u16(file + ne + 0x22);
    if (!count || !index || index > count || shift > 16 || !fits(table, (size_t)count * 8, n)) goto done;
    const uint8_t *segment = file + table + (index - 1) * 8;
    size_t start = (size_t)u16(segment) << shift;
    size_t size = u16(segment + 2);
    unsigned flags = u16(segment + 4);
    if (!size) size = DS_CAPACITY;
    if (!(flags & 1) || !(flags & 0x100) || !fits(start, size, n) || !fits(start + size, 2, n)) goto done;
    unsigned relocations = u16(file + start + size);
    size_t records = start + size + 2;
    if (!fits(records, (size_t)relocations * 8, n)) goto done;
    error_text = "NOT ENOUGH MEMORY";
    rel = calloc(DS_CAPACITY / 2, sizeof *rel);
    if (!rel) goto done;
    error_text = "INVALID TENNIS.EXE RELOCATIONS";
    for (unsigned i = 0; i < relocations; i++) {
        const uint8_t *r = file + records + i * 8;
        if ((r[1] & 3) || u16(r + 4) != index || (r[0] != 3 && r[0] != 5)) continue;
        unsigned source = u16(r + 2), target = u16(r + 6);
        if ((source & 1) || !fits(source, r[0] == 3 ? 4 : 2, size) || target >= size || !target || rel[source / 2]) goto done;
        rel[source / 2] = (uint16_t)target;
    }
    result = install(file + start, size, rel);
done:
    if (f) fclose(f);
    free(file);
    free(rel);
    return result;
}

/* Returns 1 for an ordinary DAT, 0 for a valid prepared DAT, -1 for an
 * unreadable or invalid prepared DAT. Never silently use a corrupt extension. */
int ds_load_dat(const char *path)
{
    FILE *f = fopen(path, "rb");
    uint8_t footer[FOOTER_SIZE], *payload = NULL;
    uint16_t *rel = NULL;
    int result = -1;
    ds_unload();
    error_text = "CANNOT READ TENNIS.DAT";
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END)) goto done;
    long n = ftell(f);
    if (n < (long)FOOTER_SIZE) goto done;
    if (fseek(f, n - FOOTER_SIZE, SEEK_SET) || fread(footer, 1, FOOTER_SIZE, f) != FOOTER_SIZE) goto done;
    if (memcmp(footer, "TTDSv1\r\n", 8)) { result = 1; goto done; }
    error_text = "INVALID PREPARED TENNIS.DAT";
    uint32_t base = u32(footer + 8), size = u32(footer + 12), count = u32(footer + 16);
    if (base < 6 || size != SUPPORTED_DS_SIZE || count != 101 ||
        (uint64_t)base + size + (uint64_t)count * 4 + FOOTER_SIZE != (uint64_t)n) goto done;
    size_t bytes = size + count * 4;
    error_text = "NOT ENOUGH MEMORY";
    payload = malloc(bytes);
    rel = calloc(DS_CAPACITY / 2, sizeof *rel);
    if (!payload || !rel) goto done;
    error_text = "INVALID PREPARED TENNIS.DAT";
    if (fseek(f, (long)base, SEEK_SET) || fread(payload, 1, bytes, f) != bytes ||
        hash_bytes(2166136261u, payload, bytes) != u32(footer + 20)) goto done;
    for (unsigned i = 0; i < count; i++) {
        const uint8_t *r = payload + size + i * 4;
        unsigned source = u16(r), target = u16(r + 2);
        if ((source & 1) || !fits(source, 4, size) || !target || target >= size || rel[source / 2]) goto done;
        rel[source / 2] = (uint16_t)target;
    }
    result = install(payload, size, rel);
done:
    fclose(f);
    free(payload);
    free(rel);
    return result;
}

unsigned ds_ptr_target(unsigned off)
{
    return off < DS_CAPACITY && !(off & 1) ? ds_rel[off / 2] : 0;
}

const char *ds_key_name(int sc)
{
    unsigned target = ds_ptr_target(0x466a + (sc & 0xff) * 4);
    return target ? ds_cstr(target) : "";
}

const char *ds_player_name(unsigned index)
{
    return ds_data && index < 64 ? ds_cstr(0x1074 + index * 19) : "CPU";
}
