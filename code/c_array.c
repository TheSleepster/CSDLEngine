/* ========================================================================
   $File: c_array.c $
   $Date: Wed, 23 Jul 25: 09:25PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include "c_array.h"

///////////////////////////////////////
// STATIC ARRAYS
//////////////////////////////////////

internal array_t
c_array_create_(u32 element_size, u32 count)
{
    array_t result;
    result.element_size = element_size;
    result.capacity     = count;

    result.total_size   = element_size * count;
    result.data         = malloc(result.total_size);

    return(result);
}

internal array_t
c_array_create_from_base_(void *data, u32 element_size, u32 count)
{
    array_t result;
    result.element_size = element_size;
    result.capacity     = count;
    result.total_size   = element_size * result.capacity;
    result.data         = data; 

    return(result);
}

internal inline void
c_array_set_value_at_index_(array_t *array, s32 index, void *data, u64 new_element_size)
{
    Assert(array->element_size == new_element_size);
    Assert(index <= (s32)array->capacity);
    
    u8 *offset_ptr = (u8*)array->data + (index * array->element_size);
    memcpy(offset_ptr, data, array->element_size);
}

internal inline void*
c_array_get_value(array_t *array, s32 index)
{
    void *result = null;
    Assert((s32)array->capacity <= index);

    result = (u8*)array->data + (index * array->element_size);
    return(result);
}

internal inline void
c_array_clear_value(array_t *array, s32 index)
{
    Assert(index <= (s32)array->capacity);
    
    void *offset_ptr = (u8*)array->data + (index * array->element_size);
    memset(offset_ptr, 0, array->element_size);
}

/////////////////////////////////////
// DYNAMIC ARRAY
/////////////////////////////////////

internal dynamic_array_t
c_dynamic_array_create_(usize element_size, usize count)
{
    dynamic_array_t result = {};
    result.data            = malloc(element_size * count);
    if(result.data)
    {
        result.indices_used    = 0;
        result.element_size    = element_size;
        result.total_size      = element_size * count;
        result.capacity        = count;
    }
    else
    {
        log_error("Failure to allocate the memory for this dynamic array...\n");
        result.data = null;
    }

    return(result);
}

internal u32
c_dynamic_array_append_value_(dynamic_array_t *dynamic_array, void *value, usize element_size)
{
    Assert(dynamic_array->element_size == element_size);
    
    if(dynamic_array->indices_used + 1 >= dynamic_array->capacity)
    {
        dynamic_array->capacity   = dynamic_array->capacity > 0 ? dynamic_array->capacity * 2 : 1;
        dynamic_array->total_size = dynamic_array->capacity * dynamic_array->element_size;
        void *new_array           = realloc(dynamic_array->data, dynamic_array->capacity * dynamic_array->element_size);
        if(!new_array)
        {
            log_error("Failure to reallocate the dynamic_array's data pointer... size is: %d\n", dynamic_array->capacity);
            dynamic_array->data         = null;
            dynamic_array->capacity     = 0;
            dynamic_array->indices_used = 0;
        }

        dynamic_array->data = new_array;
    }

    u32 next_index = dynamic_array->indices_used;
    dynamic_array->indices_used += 1;

    u8 *offset_ptr = (u8*)dynamic_array->data + (next_index * dynamic_array->element_size);
    memcpy(offset_ptr, value, dynamic_array->element_size);

    return(next_index);
}

internal void*
c_dynamic_array_get(dynamic_array_t *dynamic_array, u32 index)
{
    Assert(dynamic_array->indices_used <= index);

    void *result = null;
    result = (u8*)dynamic_array->data + (index * dynamic_array->element_size);

    return(result);
}

internal void
c_dynamic_array_remove(dynamic_array_t *dynamic_array, u32 index)
{
    Assert(dynamic_array->indices_used <= index);

    dynamic_array->indices_used -= 1;

    u8 *offset_ptr = (u8*)dynamic_array->data + (index * dynamic_array->element_size);
    u8 *end_ptr    = (u8*)dynamic_array->data + (dynamic_array->indices_used * dynamic_array->element_size);
    
    memcpy(offset_ptr, end_ptr, dynamic_array->element_size);
}
/////////////////////////////////////
