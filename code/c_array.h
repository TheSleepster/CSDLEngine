#if !defined(C_ARRAY_H)
/* ========================================================================
   $File: c_array.h $
   $Date: Wed, 23 Jul 25: 09:23PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_ARRAY_H
#include <stdlib.h>

#include "c_types.h"
#include "c_debug.h"

/* TODO:
 * 1.) Array For Each
 */

/////////////////////////////
// STATIC ARRAY
/////////////////////////////

typedef struct array
{
    usize  element_size;
    usize  total_size;

    u32    capacity;
    void  *data;
}array_t;

#define c_array_create(type, count)                         c_array_create_(sizeof(type), count)
#define c_array_create_from_base(data, type, count)         c_array_create_from_base(data, sizeof(type), count)
#define c_array_set_value_at_index(array, index, value_ptr) c_array_set_value_at_index_(array, index, value_ptr, sizeof(*value_ptr))

internal        array_t c_array_create_(u32 element_size, u32 count);
internal        array_t c_array_create_from_base_(void *data, u32 element_size, u32 count);
internal inline void    c_array_set_value_at_index_(array_t *array, s32 index, void *data, u64 new_element_size);
internal inline void*   c_array_get_value(array_t *array, s32 index);
internal inline void    c_array_clear_value(array_t *array, s32 index);

/////////////////////////////
// DYNAMIC ARRAY
/////////////////////////////

typedef struct dynamic_array
{
    usize indices_used;
    
    usize element_size;
    usize total_size;
    usize capacity;

    void *data;
}dynamic_array_t;

#define c_dynamic_array_create(type, count)  c_dynamic_array_create_(sizeof(type), count)
#define c_dynamic_array_append(array, value)                   \
do{                                                            \
    __typeof__(value) temp = value;                            \
    c_dynamic_array_append_value_(array, &temp, sizeof(temp)); \
}while(0)


internal dynamic_array_t c_dynamic_array_create_(usize element_size, usize count);
internal void            c_dynamic_array_destroy(dynamic_array_t *array);
internal u32             c_dynamic_array_append_value_(dynamic_array_t *dynamic_array_t, void *value, usize element_size);
internal void*           c_dynamic_array_get(dynamic_array_t *dynamic_array, u32 index);
internal void            c_dynamic_array_remove(dynamic_array_t *dynamic_array, u32 index);

/////////////////////////////

#endif
