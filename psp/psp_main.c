/* PSP module description: user-mode eboot, use all the free memory for the heap */
#include <pspkernel.h>

PSP_MODULE_INFO("TopTennis", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);
