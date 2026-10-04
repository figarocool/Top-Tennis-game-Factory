/* Run under sanitizers against local malformed files and original-data inputs. */
#include "../src/dsimg.h"
#include "../src/pak.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        int result = ds_load(argv[i]);
        if (result == 0) { assert(ds_data && ds_size == 19876); }
        else assert(!ds_data && !ds_size);
        result = ds_load_dat(argv[i]);
        if (result == 0) { assert(ds_data && ds_size == 19876); }
        else assert(!ds_data && !ds_size);
        pak_open(argv[i]);
        pak_close(); ds_unload();
    }
    puts("sanitizer reader checks: OK");
    return 0;
}
