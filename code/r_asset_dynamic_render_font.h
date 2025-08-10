#if !defined(ASSET_DYNAMIC_FONT_H)
/* ========================================================================
   $File: asset_dynamic_font.h $
   $Date: Sat, 09 Aug 25: 11:23AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ASSET_DYNAMIC_FONT_H
typedef struct dynamic_render_font
{
    memory_arena_t  font_arena;
    string_t        loaded_data;

    dynamic_array_t pixel_sizes;
}dynamic_render_font_t;

typedef struct dynamic_render_font_varient dynamic_render_font_varient_t;
typedef struct dynamic_render_font_page
{
    bool8                            page_full;
    bool8                            bitmap_dirty;

    hash_table_t                     glyph_lookup;
    texture2D_t                      font_atlas;

    dynamic_render_font_varient_t   *owner_varient;
    struct dynamic_render_font_page *next_page;
}dynamic_render_font_page_t;

typedef struct dynamic_render_font_varient
{
    s64                         pixel_size;
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

#endif
