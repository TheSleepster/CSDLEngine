#if !defined(C_HASH_TABLE_H)
/* ========================================================================
   $File: c_hash_table.h $
   $Date: Sat, 02 Aug 25: 02:35AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_HASH_TABLE_H
#include "c_types.h"
#include "c_string.h"
#include "c_array.h"
#include "c_debug.h"

const u64 default_fnv_hash_value = 14695981039346656037ULL;

// NOTE(Sleepster): The key is simply a string because at the end of the day a string is just a byte array anyway... 
typedef struct hash_table_entry
{
    string_t key;
    void    *value;
}hash_table_entry_t;

typedef struct hash_table
{
    hash_table_entry_t *entries;
    u32                 max_entries;

    usize               value_size;
    u32                 entry_counter;
}hash_table_t;

internal inline hash_table_t c_hash_table_create_(void *memory, u32 max_entries, usize value_size);
internal        void         c_hash_insert_kv_pair_(hash_table_t *table, string_t key, void *value, usize value_size);
internal        void*        c_hash_get_value(hash_table_t *table, string_t key);

#define c_hash_table_create(memory, max_entries, data_type) c_hash_table_create_(memory, max_entries, sizeof(data_type))
#define c_hash_insert_kv_pair(table, key, value)            c_hash_insert_kv_pair_(table, key, (void*)value, sizeof(value))

#endif
