#if !defined(C_MULTITHREADING_WORK_QUEUE_H)
/* ========================================================================
   $File: c_multithreading_work_queue.h $
   $Date: Sat, 30 Aug 25: 10:01PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_MULTITHREADING_WORK_QUEUE_H
#include "c_base.h"
#include "c_types.h"

/* TODO:
 * - [ ] Parallel for loop
 * - [ ] Experiment with lambdas
 */

typedef struct os_semaphore os_semaphore_t;
typedef struct os_thread    os_thread_t;

#define WORK_QUEUE_ENTRY_CALLBACK(name) void name(void *user_data);
typedef WORK_QUEUE_ENTRY_CALLBACK(work_queue_callback_t);

typedef struct multithreading_work_queue_entry
{
    void                  *user_data;
    work_queue_callback_t *callback;
}multithreading_work_queue_entry_t;

typedef struct multithreading_work_queue
{
    u32 volatile                      completion_goal;
    u32 volatile                      next_entry_to_read;
    u32 volatile                      next_entry_to_write;
    u32 volatile                      total_work_entries_completed;

    multithreading_work_queue_entry_t entries[512];
    os_semaphore_t                    semaphore;
}multithreading_work_queue_t;

typedef struct thread_pool
{
    os_thread_t *threads;
    u32          thread_count;
}thread_pool_t;

typedef struct multithreading_work_queue_manager
{
    multithreading_work_queue_t high_priority_queue;
    multithreading_work_queue_t low_priority_queue;

    thread_pool_t               thread_pool;
}multithreading_work_queue_manager_t;

internal void  s_work_queue_manager_init(multithreading_work_queue_manager_t *manager);
internal bool8 s_work_queue_do_next_work_entry(multithreading_work_queue_t *queue);
internal void  s_work_queue_add_entry(multithreading_work_queue_t *queue, work_queue_callback_t *callback, void *user_data);
internal void  s_work_queue_finish_all_work(multithreading_work_queue_t *queue);

#endif
