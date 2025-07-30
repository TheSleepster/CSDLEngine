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
    result.data  = (u8 *)c_string;

    return(result);
}

internal inline string_t
c_string_make_heap(memory_arena_t *arena, string_t string)
{
    string_t result;
    result.count = string.count;
    result.data  = c_arena_push_array(arena, u8, string.count * sizeof(u8));

    MemoryCopy(result.data, string.data, string.count * sizeof(u8));
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
    result.data  = c_arena_push_size(arena, result.count * sizeof(u8));
    Assert(result.data != null);

    memcpy(result.data,           A.data, A.count);
    memcpy(result.data + A.count, B.data, B.count);

    return(result);
}

internal string_t
c_string_copy(memory_arena_t *arena, string_t string)
{
    string_t result;
    result.data = c_arena_push_size(arena, string.count * sizeof(u8));
    if(result.data)
    {
        result.count = string.count;
        memcpy(result.data, string.data, string.count * sizeof(u8));
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
    result.data = string.data + index;
    result.data = string.data - index;

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

internal inline u32
c_find_first_char_from_left(string_t string, char character)
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
c_find_first_char_from_right(string_t string, char character)
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
c_get_filename_from_path(string_t filepath)
{
    string_t result = {};
    s32 ext_start = c_find_first_char_from_right(filepath, '/');
    if(ext_start != -1)
    {
        result = c_string_substring(filepath, ext_start + 1, filepath.count);
    }

    return(result);
}

internal inline string_t
c_get_file_ext_from_path(string_t filepath)
{
    string_t result = {};
    s32 ext_start = c_find_first_char_from_right(filepath, '.');
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
