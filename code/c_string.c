/* ========================================================================
   $File: c_string.c $
   $Date: Wed, 23 Jul 25: 12:56PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include "c_string.h"

internal s32
c_string_length(const char *c_string)
{
    Assert(c_string);

    s32 result = 0;
    while(*c_string != 0) 
    {
        ++result;
        ++c_string;
    }

    return(result);
}

internal inline string_t
c_string_create(const char *c_string)
{
    string_t result;
    result.count = c_string_length(c_string);
    result.data  = (byte*)c_string;

    return(result);
}

internal inline string_t
c_string_make_heap(memory_arena_t *arena, string_t string)
{
    string_t result;
    result.count = string.count;
    result.data  = c_arena_push_array(arena, byte, string.count * sizeof(byte));

    MemoryCopy(result.data, string.data, string.count * sizeof(byte));
    return(result);
}

internal inline bool8
c_string_compare(string_t A, string_t B)
{
    if(A.count != B.count) return false;
    return(memcmp(A.data, B.data, A.count) == 0);
}

internal string_t
c_string_concat(memory_arena_t *arena, string_t A, string_t B)
{
    string_t result;
    result.count = A.count + B.count;
    result.data  = c_arena_push_size(arena, result.count * sizeof(byte));
    Assert(result.data != null);

    memcpy(result.data,           A.data, A.count);
    memcpy(result.data + A.count, B.data, B.count);

    return(result);
}

internal string_t
c_string_make_copy(memory_arena_t *arena, string_t string)
{
    string_t result;
    result.data = c_arena_push_size(arena, string.count * sizeof(byte));
    if(result.data)
    {
        result.count = string.count;
        memcpy(result.data, string.data, string.count * sizeof(byte));
    }
    else
    {
        log_error("Failed to allocate string memory...\n");
    }

    return(result);
}

internal string_t
c_string_sub_from_left(string_t string, u32 index)
{
    string_t result;
    result.data  = string.data + index;
    result.count = string.count - index;

    return(result);
}

internal string_t
c_string_sub_from_right(string_t string, u32 index)
{
    string_t result;
    result.data  = (string.data + string.count) - index;
    result.count = index;

    return(result);
}

internal string_t
c_string_substring(string_t string, u32 first_index, u32 last_index)
{
    string_t result;
    result.data  = (string.data + first_index);
    result.count = last_index - first_index;

    return(result);
}

internal inline void
c_string_advance_by(string_t *string, u32 amount)
{
    Assert(amount < string->count);
    string->data  += amount;
    string->count -= amount;
}

internal inline u32
c_string_find_first_char_from_left(string_t string, char character)
{
    u32 result = -1;
    for(u32 index = 0;
        index < string.count;
        ++index)
    {
        char found = (char)string.data[index];
        if(found == character)
        {
            result = index;
            break;
        }
    }

    return(result);
}

internal inline u32
c_string_find_first_char_from_right(string_t string, char character)
{
    u32 result = -1;
    for(u32 index = string.count;
        index > 0;
        --index)
    {
        char found = (char)string.data[index];
        if(found == character)
        {
            result = index;
            break;
        }
    }

    return(result);
}

internal inline string_t
c_string_get_filename_from_path(string_t filepath)
{
    string_t result = {};
    s32 ext_start = c_string_find_first_char_from_right(filepath, '/');
    if(ext_start != -1)
    {
        result = c_string_substring(filepath, ext_start + 1, filepath.count);
    }

    return(result);
}

internal inline string_t
c_string_get_file_ext_from_path(string_t filepath)
{
    string_t result = {};
    s32 ext_start = c_string_find_first_char_from_right(filepath, '.');
    if(ext_start != -1)
    {
        result = c_string_substring(filepath, ext_start, filepath.count);
    }

    return(result);
}

internal inline const char *
c_string_to_const_array(string_t string)
{
    return((const char *)string.data);
}


///////////////////
// STRING BUILDER
///////////////////

// TODO(Sleepster): c_string_builder_deinit 

internal inline string_builder_buffer_t*
c_string_builder_get_base_buffer(string_builder_t *builder)
{
    return((string_builder_buffer_t*)builder->initial_bytes);
}

internal void
c_string_builder_init(string_builder_t *builder, usize new_buffer_size)
{
    Assert(builder->is_initialized == false);
    
    builder->total_allocated = 0;
    builder->new_buffer_size = new_buffer_size;
    memset(builder->initial_bytes, 0, STRING_BUILDER_BUFFER_SIZE);

    string_builder_buffer_t *buffer = c_string_builder_get_base_buffer(builder);
    buffer->bytes_allocated         = STRING_BUILDER_BUFFER_SIZE;
    buffer->bytes_used              = 0;
    buffer->next_buffer             = null;

    builder->current_buffer   = buffer;
    builder->total_allocated += buffer->bytes_allocated;

    builder->is_initialized   = true;
}

internal inline string_builder_buffer_t*
c_string_builder_get_current_buffer(string_builder_t *builder)
{
    if(builder->current_buffer) return(builder->current_buffer);
    return(c_string_builder_get_base_buffer(builder));
}

internal inline byte*
c_string_builder_get_buffer_data(string_builder_buffer_t *buffer)
{
    return((byte*)buffer + sizeof(string_builder_buffer_t));
}

internal bool8
c_string_builder_create_new_buffer(string_builder_t *builder)
{
    bool8 result = false;
    usize new_size = builder->new_buffer_size > 0 ? builder->new_buffer_size : STRING_BUILDER_BUFFER_SIZE;

    byte *bytes = malloc(new_size);
    if(bytes)
    {
        ZeroMemory(bytes, new_size);

        string_builder_buffer_t *buffer = bytes;
        buffer->next_buffer     = null;
        buffer->bytes_used      = 0;
        buffer->bytes_allocated = new_size - sizeof(string_builder_buffer_t);

        string_builder_buffer_t *last_buffer = c_string_builder_get_current_buffer(builder);
        last_buffer->next_buffer  = buffer;
        builder->current_buffer   = buffer; 
        builder->total_allocated += buffer->bytes_allocated;

        result = true;
    }

    return(result);
}

internal void 
c_string_builder_append(string_builder_t *builder, string_t data)
{
    if(!builder->is_initialized) c_string_builder_init(builder, STRING_BUILDER_BUFFER_SIZE);

    byte *bytes  = data.data;
    u32 length = data.count;
    while(length > 0)
    {
        string_builder_buffer_t *buffer = c_string_builder_get_current_buffer(builder);
        u32 length_max = buffer->bytes_allocated - buffer->bytes_used;
        if(length_max <= 0)
        {
            bool8 success = c_string_builder_create_new_buffer(builder);
            if(!success)
            {
                log_error("Failure to expand the string builder...\n");
                return;
            }

            buffer = builder->current_buffer;
            Assert(buffer != null);

            length_max = buffer->bytes_allocated - buffer->bytes_used;
            Assert(length_max > 0);
        }

        u32 bytes_to_copy = Min(length, length_max);
        memcpy(c_string_builder_get_buffer_data(buffer) + buffer->bytes_used, bytes, bytes_to_copy);

        length -= bytes_to_copy;
        bytes  += bytes_to_copy;

        buffer->bytes_used += bytes_to_copy;
    }
}

internal inline void
c_string_builder_append_value(string_builder_t *builder, void *value_ptr, byte len)
{
    string_t builder_string;
    builder_string.data  = value_ptr;
    builder_string.count = len;
    c_string_builder_append(builder, builder_string);
}

internal string_t
c_string_builder_get_string(string_builder_t *builder)
{
    string_t result = {};
    string_builder_buffer_t *buffer = c_string_builder_get_current_buffer(builder);

    while(buffer)
    {
        string_t temp_string;
        temp_string.data  = c_string_builder_get_buffer_data(buffer);
        temp_string.count = buffer->bytes_used;

        result = c_string_concat(&global_context.temporary_arena, result, temp_string);

        buffer = buffer->next_buffer;
    }

    return(result);
}

internal void 
c_string_builder_write_to_file(file_t *file, string_builder_t *builder)
{
    string_t string_to_write = c_string_builder_get_string(builder);
    Assert(file->for_writing);

    c_file_write_string(file, string_to_write);
}

internal s32
c_string_builder_get_string_length(string_builder_t *builder)
{
    s32 result = 0;
    string_builder_buffer_t *buffer = c_string_builder_get_base_buffer(builder);
    while(buffer)
    {
        result += buffer->bytes_used;
        buffer = buffer->next_buffer;
    }

    return(result);
}
