/* ========================================================================
   $File: s_multithreading_work_queue.cpp $
   $Date: Sat, 30 Aug 25: 10:06PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_multithreading_work_queue.h"
#include "os_platform_file.h"

internal void 
s_work_queue_manager_init(multithreading_work_queue_manager_t *manager)
{
    s32 user_thread_count    = os_get_cpu_count();
    s32 threads_to_open      = user_thread_count / 2;
    os_semaphore_t semaphore = os_semaphore_create(0, threads_to_open);

    manager->high_priority_queue.semaphore = semaphore;
    manager->low_priority_queue.semaphore  = semaphore;

    for(s32 thread_index = 0;
        thread_index < threads_to_open;
        ++thread_index)
    {
        os_thread_t thread_data = os_thread_create(os_work_queue_entry_proc, manager, false);
        os_thread_close_handle(&thread_data);
    }
}

internal void 
s_work_queue_add_entry(multithreading_work_queue_t *queue, work_queue_callback_t *callback, void *user_data)
{
    u32 this_entry_to_write = queue->next_entry_to_write;
    u32 next_entry_to_write = (this_entry_to_write + 1) % ArrayCount(queue->entries);

    ReadWriteBarrier;
    AtomicCompareExchange32(&queue->next_entry_to_write,
                            next_entry_to_write,
                            this_entry_to_write);
    AtomicIncrement32(&queue->completion_goal);
    
    multithreading_work_queue_entry_t *new_entry = queue->entries + this_entry_to_write;
    new_entry->callback  = callback;
    new_entry->user_data = user_data;
    new_entry->is_valid  = true;

    os_semaphore_release(&queue->semaphore, 1);
}

internal bool8
s_work_queue_do_next_work_entry(multithreading_work_queue_t *queue)
{
    bool8 should_sleep = false;
    u32 unincremented_entry_to_read = queue->next_entry_to_read;
    u32 next_entry_to_read = (unincremented_entry_to_read + 1) % ArrayCount(queue->entries);

    if(unincremented_entry_to_read != queue->next_entry_to_write)
    {
        u32 work_entry_index = AtomicCompareExchange32(&queue->next_entry_to_read,
                                                        next_entry_to_read,
                                                        unincremented_entry_to_read);
        if(work_entry_index == unincremented_entry_to_read)
        {
            multithreading_work_queue_entry_t *entry = queue->entries + work_entry_index;
            if(entry->is_valid)
            {
                entry->is_valid = false;
                entry->callback(entry->user_data);

                AtomicIncrement32(&queue->total_work_entries_completed);
            }
        }
    }
    else
    {
        should_sleep = true;
    }

    return(should_sleep);
}

internal void
s_work_queue_finish_all_work(multithreading_work_queue_t *queue)
{
    while(!s_work_queue_do_next_work_entry(queue));

    AtomicExchange32(&queue->completion_goal, 0);
    AtomicExchange32(&queue->total_work_entries_completed, 0);
}

/* NOTE(Sleepster):
   We have to put this here for some reason... It really bugs me.
   All OS code should just go in the associated OS file.
   But this just can't.

   TODO: Maybe switch to SDL thread?
*/
#if OS_WINDOWS
DWORD WINAPI
os_work_queue_entry_proc(void *lpParam)
{
    multithreading_work_queue_manager_t *work_queue_manager = (multithreading_work_queue_manager_t*)lpParam;
    for(;;)
    {
        if(s_work_queue_do_next_work_entry(&work_queue_manager->high_priority_queue))
        {
            if(s_work_queue_do_next_work_entry(&work_queue_manager->low_priority_queue))
            {
                os_semaphore_wait(&work_queue_manager->high_priority_queue.semaphore, 0);
                log_info("Thread Sleeping...\n");
            }
        }
    }
}
#elif OS_LINUX | OS_MAC
int
os_work_queue_entry_proc(void *lpParam)
{
    multithreading_work_queue_manager_t *work_queue_manager = (multithreading_work_queue_manager_t*)lpParam;
    for(;;)
    {
        if(s_work_queue_do_next_work_entry(&work_queue_manager->high_priority_queue))
        {
            if(s_work_queue_do_next_work_entry(&work_queue_manager->low_priority_queue))
            {
                os_semaphore_wait(&work_queue_manager->high_priority_queue.semaphore, 0);
                log_info("Thread Sleeping...\n");
            }
        }
    }
}

#else
#error "Threading for this OS is not supported..."
#endif
