/* Initialised data segment (DS = 1030) of the original TENNIS.EXE.
 * The game keeps many constant tables there (animation frames, menu strings, player names...).
 * We read them straight from the user's original executable so no game data is redistributed. */
#ifndef DSIMG_H
#define DSIMG_H
#include <stdint.h>
#include <stddef.h>

int  ds_load(const char *exe_path);
extern uint8_t *ds_data;
extern size_t   ds_size;

static inline uint8_t  ds_u8 (unsigned off) { return ds_data[off]; }
static inline int8_t   ds_s8 (unsigned off) { return (int8_t)ds_data[off]; }
static inline uint16_t ds_u16(unsigned off) { return ds_data[off] | ds_data[off + 1] << 8; }
static inline int16_t  ds_s16(unsigned off) { return (int16_t)ds_u16(off); }
/* Pointers stored in the initialised data are patched by the NE loader; this maps a DS offset holding a
 * (near or far) pointer to the DS offset it refers to (0 if there is no relocation there). */
unsigned ds_ptr_target(unsigned off);
const char *ds_key_name(int scancode);       /* name table at DS:466A (4 bytes per scancode) */

/* NUL-terminated string at DS:off (the game uses ASCIIZ strings for file names and messages) */
static inline const char *ds_cstr(unsigned off) { return (const char *)(ds_data + off); }
#endif
