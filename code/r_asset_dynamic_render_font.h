#if !defined(ASSET_DYNAMIC_FONT_H)
/* ========================================================================
   $File: asset_dynamic_font.h $
   $Date: Sat, 09 Aug 25: 11:23AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ASSET_DYNAMIC_FONT_H
#include "r_asset_texture.h"
#include "c_math.h"

typedef struct dynamic_render_font_varient dynamic_render_font_varient_t;

/*===========================================
  ================= UNICODE =================
  ===========================================*/

u8 UTF8_trailing_bytes[] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

u8 UTF8_initial_bytemask[] = {0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
u8 UTF8_first_byte_mask[]  = {0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};

u32 UTF8_offsets[] = {0x00000000, 0x00003080, 0x000e2080, 
                      0x03c82080, 0xfa082080, 0x82082080};

#define  UTF16_MAX_CHARACTER          0x0010FFFF
#define  UTF32_MAX_CHARACTER          0x7FFFFFFF
#define  UTF32_REPLACEMENT_CHARACTER  0x0000FFFD
////////////////////////////////

typedef struct dynamic_render_font
{
    string_t        filename;

    bool8           is_valid;
    bool8           is_initialized;
    
    FT_Face         font_face;
    
    memory_arena_t  font_arena;
    string_t        loaded_data;

    dynamic_array_t pixel_sizes;
}dynamic_render_font_t;

typedef struct dynamic_render_font_page
{
    bool8                            is_full;
    bool8                            bitmap_valid;
    bool8                            bitmap_dirty;

    hash_table_t                     glyph_lookup;
    texture2D_t                      font_atlas;

    s32                              bitmap_cursor_x;
    s32                              bitmap_cursor_y;

    dynamic_render_font_varient_t   *owner_varient;
    struct dynamic_render_font_page *next_page;
}dynamic_render_font_page_t;

typedef struct dynamic_render_font_varient
{
    s64                         pixel_size;

    s64                         line_spacing;
    s64                         max_ascender;
    s64                         max_descender;
    s32                         y_center_offset;
    s32                         typical_ascender;
    s32                         typical_descender;
    s32                         em_width;
    s32                         default_unknown_character;
    s32                         default_utf32_unknown_character;

    dynamic_render_font_t      *parent;
    dynamic_render_font_page_t *first_page;
}dynamic_render_font_varient_t;

typedef struct font_glyph
{
    vec2_t   atlas_offset;
    vec2_t   glyph_size;
    vec2_t   glyph_render_size;

    string_t hash_key;

    // RENDERING //
    s32 offset_x;
    s32 offset_y;

    s32 advance;
    s32 ascent;
    ////////////////

    dynamic_render_font_page_t *owner_page;
}font_glyph_t;

internal inline s32
FT_ROUND(s32 X)
{
    if (X >= 0) return (X + 0x1f) >> 6;
    return -(((-X) + 0x1f) >> 6);
}

internal u8*
unicode_next_character(u8 *character)
{
    u8 character_bytes = 1 + UTF8_trailing_bytes[*character];
    return(character + character_bytes);
}

#endif
