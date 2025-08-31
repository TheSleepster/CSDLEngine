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
    
internal inline s32            os_get_cpu_count();
internal        os_semaphore_t os_semaphore_create(s32 initial_thread_count, s32 max_thread_count);
internal inline void           os_semaphore_close(os_semaphore_t *semaphore);
internal inline s32            os_semaphore_release(os_semaphore_t *semaphore, s32 threads_to_release);
internal inline bool8          os_semaphore_destroy(os_semaphore_t *semaphore);
internal        os_thread_t    os_thread_create(void *thread_proc, void *user_data, bool8 close_handle);
internal inline void           os_thread_wait(os_semaphore_t *semaphore, u64 wait_duration_ms);
internal inline bool8          os_thread_close_handle(os_thread_t *thread_data);
internal inline os_mutex_t     os_mutex_create();
internal inline void           os_mutex_free(os_mutex_t *mutex);
internal inline bool8          os_mutex_lock(os_mutex_t *mutex);
internal inline bool8          os_mutex_unlock(os_mutex_t *mutex);

#endif
