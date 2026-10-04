/* newlib heap for the Vita: the default is far too small for the sprite and page buffers */
int _newlib_heap_size_user = 96 * 1024 * 1024;
