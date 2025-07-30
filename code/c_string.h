#if !defined(C_STRING_H)
/* ========================================================================
   $File: c_string.h $
   $Date: Wed, 02 Jul 25: 06:12PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_STRING_H

#include "c_types.h"

typedef struct string
{
    u8  *data;
    u32  count;
}string_t;

//////////// API DEFINITIONS //////////////
internal        s32         c_string_length(const char *c_string);
internal inline string_t    c_string_create(const char *c_string);
internal inline string_t    c_string_make_heap(memory_arena_t *arena, string_t string);
internal inline bool8       c_string_compare(string_t A, string_t B);
internal inline string_t    c_string_concat(memory_arena_t *arena, string_t A, string_t B);
internal inline const char *c_string_to_const_array(string_t string);

internal        string_t c_string_copy(memory_arena_t *arena, string_t string);
internal        string_t c_string_sub_from_left(string_t string, u32 index);
internal        string_t c_string_sub_from_right(string_t string, u32 index);
internal        string_t c_string_substring(string_t string, u32 first_index, u32 last_index);

internal inline u32         c_find_first_char_from_left(string_t string,  char character);
internal inline u32         c_find_first_char_from_right(string_t string, char character);
internal inline string_t    c_get_filename_from_path(string_t filepath);
internal inline string_t    c_get_file_ext_from_path(string_t filepath);

// MACROS
#define STR(x)   (string_t){.data = (u8 *)x, .count = c_string_length(x)}
#define C_STR(x) ((const char *)x.data);
///////////////////////////////////////////

#endif
