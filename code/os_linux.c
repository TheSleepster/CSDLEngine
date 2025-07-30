/* ========================================================================
   $File: os_linux.c $
   $Date: Wed, 30 Jul 25: 12:27PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include "c_types.h"
#include "c_base.h"

#include <sys/mman.h>
#include <errno.h>

internal inline void*
os_allocate_memory(usize allocation_size)
{
    errno = 0;
    
    void *data = mmap(0, allocation_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(errno == MAP_FAILED)
    {
        int error = errno;
        log_fatal("mmap failed... error: (%s), code: '%d'...\n", strerror(error), error);

        data = null;
    }

    return(data);
}

internal inline void
os_free_memory(void *data, usize free_size)
{
    if(munmap(data, free_size) == -1)
    {
        int error = errno;
        log_fatal("munmap failed... error: (%s), code: '%d'...\n", strerror(error), error);
    }
}
