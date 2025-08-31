#if !defined(C_MULTITHREADING_PRIMITIVES_H)
/* ========================================================================
   $File: c_multithreading_primitives.h $
   $Date: Sun, 31 Aug 25: 02:39PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_MULTITHREADING_PRIMITIVES_H
#include "c_base.h"
#include "c_types.h"

#if OS_WINDOWS
     typedef void* os_handle_t;
#elif OS_LINUX
     typedef s32 os_handle_t;
#elif OS_MAC
#endif

typedef struct os_thread
{
    os_handle_t handle;
    u32         thread_id;
    void       *user_data;
}os_thread_t;

typedef struct os_mutex
{
    os_handle_t handle;
}os_mutex_t;

typedef struct os_semaphore
{
    os_handle_t handle;
}os_semaphore_t;

#endif
