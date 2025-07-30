/* ========================================================================
   $File: os_windows.c $
   $Date: Mon, 28 Jul 25: 07:47PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#include <windows.h>

#include "c_types.h"
#include "c_base.h"

internal inline void*
os_allocate_memory(usize allocation_size)
{
    return(VirtualAlloc(0, allocation_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE));
}

internal inline void
os_free_memory(void *data, usize free_size)
{
    VirtualFree(data, 0, MEM_RELEASE);
}
