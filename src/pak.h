/* TENNIS.DAT archive: u16 count, u32 dir_pos, ... ; directory = count * 64-byte entries
 * (char path[56] NUL-terminated, u32 size, u32 offset). Original: unit 1020 (1020:0a2b lookup). */
#ifndef PAK_H
#define PAK_H
#include <stdint.h>
#include <stddef.h>

int      pak_open(const char *path);
void     pak_close(void);
int      pak_count(void);
const char *pak_name(int idx);
/* Returns malloc'd copy of the file (caller frees) or NULL. Name is matched case-insensitively,
 * '/' and '\\' are equivalent. */
uint8_t *pak_load(const char *name, size_t *size);
#endif
