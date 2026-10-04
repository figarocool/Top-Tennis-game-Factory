#include "original.h"
#include <dirent.h>
#include <stdio.h>
#include <strings.h>

int original_path(char *out, size_t capacity, const char *dir, const char *name)
{
    int n = snprintf(out, capacity, "%s/%s", dir, name);
    if (n < 0 || (size_t)n >= capacity) return -1;
    /* Some console mounts allow file access but not directory enumeration. */
    FILE *exact = fopen(out, "rb");
    if (exact) { fclose(exact); return 0; }
    DIR *folder = opendir(dir);
    if (!folder) return -1;
    int found = -1;
    struct dirent *entry;
    while ((entry = readdir(folder))) {
        if (strcasecmp(entry->d_name, name)) continue;
        n = snprintf(out, capacity, "%s/%s", dir, entry->d_name);
        found = n >= 0 && (size_t)n < capacity ? 0 : -1;
        break;
    }
    closedir(folder);
    return found;
}
