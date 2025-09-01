/* ========================================================================
   $File: DEBUG_core.cpp $
   $Date: Mon, 01 Sep 25: 01:07PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "c_base.h"
#include "c_types.h"
#include "c_log_assert.h"

/* TODO(Sleepster):
   This is like 1/4 of the way done, I just need to create DEBUG gui first with a new renderer...
 */

#define DEBUG_TIMED_BLOCK__(number, ...) timed_block_t DEBUG_timed_block_##number(__FILE__, __FUNCTION__, __COUNTER__, __LINE__)
#define DEBUG_TIMED_BLOCK_(number, ...)  DEBUG_TIMED_BLOCK__(number, ##__VA_ARGS__) 
#define DEBUG_TIMED_BLOCK(...)           DEBUG_TIMED_BLOCK_(__LINE__, ##__VA_ARGS__)

struct DEBUG_record_data_t
{
    char *filename;
    char *block_name;

    u32   line_number;
    u32   hit_count;
    u64   total_cycle_count;
};

extern DEBUG_record_data_t DEBUG_records[];

struct timed_block_t
{
    DEBUG_record_data_t *current_record;

    timed_block_t(char *filename, char *block_name, s32 record_index, s32 line_number, s32 hit_count = 1)
    {
        current_record = DEBUG_records + record_index;
        current_record->filename           = filename;
        current_record->block_name         = block_name;
        current_record->line_number        = line_number;
        current_record->hit_count         += hit_count;
        current_record->total_cycle_count -= rdtsc();
    }

   ~timed_block_t()
    {
        current_record->total_cycle_count += rdtsc();
    }
};
