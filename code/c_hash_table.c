/* ========================================================================
   $File: c_hash_table.c $
   $Date: Sat, 02 Aug 25: 02:35AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "c_hash_table.h"

internal u64 
c_fnv_hash_value(u8 *data, usize element_size, u64 hash_value)
{
    u64 fnv_prime  = 1099511628211ULL;
    u64 new_value  = hash_value;
        
    for(u32 byte_index = 0;
        byte_index < element_size;
        ++byte_index)
    {
        new_value = (new_value ^ data[byte_index]) * fnv_prime;
    }

    return(new_value);
}


internal inline hash_table_t
c_hash_table_create_(void *memory, u32 max_entries, usize key_size, usize value_size)
{
    hash_table_t result;
    result.entries       = memory;
    result.max_entries   = max_entries;
    result.key_size      = key_size;
    result.value_size    = value_size;
    result.entry_counter = 0;

    return(result);
}

internal inline u64
c_hash_create_key_index(hash_table_t *table, void *key, usize key_size)
{
    u64 result = 0;
    u64 new_hash_value = c_fnv_hash_value((u8*)key, key_size, default_fnv_hash_value);

    result = (new_hash_value % table->max_entries + table->max_entries) % table->max_entries;
    return(result);
}

internal void
c_hash_insert_kv_pair_(hash_table_t *table, void *key, void *value, usize key_size, usize value_size)
{
    Assert(table->key_size   == key_size);
    Assert(table->value_size == value_size);

    u64 hash_index = c_hash_create_key_index(table, key, key_size);
    Assert(hash_index >= 0);

    hash_table_entry_t *entry = &table->entries[hash_index];
    Assert(entry);
    
    if(entry->key != null)
    {
        if(memcmp(entry->key, key, table->key_size) == 0)
        {
            entry->value = value;
            log_warning("hash table value at index: '%d' has been updated...\n");

            return;
        }
    }
    entry->key   = key;
    entry->value = value;

    table->entry_counter += 1;
}

internal void*
c_hash_get_value_(hash_table_t *table, void *key, usize key_size)
{
    void *result = null;
    
    Assert(key_size == table->key_size);
    u64 hash_index = c_hash_create_key_index(table, key, table->key_size);
    Assert(hash_index >= 0);

    hash_table_entry_t *entry = &table->entries[hash_index];
    if(entry->key)
    {
        if(memcmp(entry->key, key, table->key_size) == 0)
        {
            result = entry->value;
        }
    }
    return(result);
}

internal void
c_hash_clear_value_at_index(hash_table_t *table, s32 index)
{
    Assert(index >= 0);
    Assert(table->max_entries >= (u32)index);

    hash_table_entry_t *entry = &table->entries[index];
    entry->value = null;
}

internal void
c_hash_clear_index(hash_table_t *table, s32 index)
{
    Assert(index >= 0);
    Assert(table->max_entries >= (u32)index);
    
    hash_table_entry_t *entry = &table->entries[index];
    entry->value = null;
    entry->key   = null;
}

internal void
c_hash_clear_table_entries(hash_table_t *table)
{
    for(u32 hash_index = 0;
        hash_index < table->max_entries;
        ++hash_index)
    {
        hash_table_entry_t *entry = &table->entries[hash_index];
        entry->value = null;
        entry->key   = null;
    }
}
