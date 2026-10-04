#ifndef ORIGINAL_H
#define ORIGINAL_H
#include <stddef.h>
/* Resolve filenames regardless of the case used when extracting the original game. */
int original_path(char *out, size_t capacity, const char *dir, const char *name);
#endif
