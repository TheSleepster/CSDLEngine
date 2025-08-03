#if !defined(C_FILE_API_H)
/* ========================================================================
   $File: c_file_api.h $
   $Date: Fri, 25 Jul 25: 01:24PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_FILE_API_H
#include "c_types.h"
#include "c_string.h"

// TODO(Sleepster): ASYNC THREAD SAFE 

internal string_t c_read_entire_file(string_t filepath);
internal string_t c_read_entire_file_arena(memory_arena_t *arena, string_t filepath);
internal string_t c_read_entire_file_za(zone_allocator_t *zone, string_t filepath, za_allocation_tag_t tag);
internal void     c_write_entire_file(string_t filepath, void *data, s64 bytes_to_write, bool8 overwrite);
#endif
