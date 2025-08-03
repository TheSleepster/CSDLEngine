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

typedef struct hash_table_entry
{
    void *key;
    void *value;
}hash_table_entry_t;

typedef struct hash_table
{
    hash_table_entry_t *entries;
    u32                 max_entries;

    usize               key_size;
    usize               value_size;
    u32                 entry_counter;
}hash_table_t;

#define c_hash_table_create(memory, max_entries, key_type, data_type) c_hash_table_create_(memory, max_entries, sizeof(key_type), sizeof(data_type))
#define c_hash_insert_kv_pair(table, key, value)                      c_insert_kv_pair_(table, (void*)key, (void*)value, sizeof(key), sizeof(value))
#define c_hash_get_value(table, key)                                  c_hash_get_value(table, (void*)key, sizeof(key))

#endif
