#if !defined(R_RENDERER_DATA_H)
/* ========================================================================
   $File: r_renderer_data.h $
   $Date: Wed, 23 Jul 25: 10:59PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_RENDERER_DATA_H

#define MAX_QUADS    (2500)
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_LINES    (MAX_VERTICES / 2)
#define MAX_INDICES  (MAX_QUADS * 6)

#include "c_types.h"
#include "c_math.h"

#include "r_asset_shader.h"

////////////////////////////////////////
// RENDERING PRIMITIVES 
////////////////////////////////////////

#pragma pack(push, 1)
typedef struct vertex
{
    vec3_t vPosition;
    vec2_t vUVData;
    vec4_t vColor;
    vec3_t vVSNormals;
    u32    vTextureIndex;
}vertex_t;
#pragma pack(pop)

typedef enum render_quad_options
{
    RQO_NONE         = 0X00,
    RQO_SHADOWCASTER = 0X01,
    RQO_EMISIVE      = 0x02,
    RQO_COUNT,
}render_quad_options_t;

typedef struct render_quad
{
    union
    {
        struct
        {
            vertex_t top_left;
            vertex_t top_right;
            vertex_t bottom_left;
            vertex_t bottom_right;
        };
        vertex_t elements[4];
    };

    bool8                 culled;
    vec2_t                center_pos;
    float32               layer_depth;
    
    u32                   texture_id;
    render_quad_options_t options;
}render_quad_t;

typedef struct render_line
{
    union
    {
        struct
        {
            vertex_t start_point;
            vertex_t end_point;
        };
        vertex_t elements[2];
    };

    bool8   culled;
    float32 layer_depth;
}render_line_t;

///////////////////////
// RENDER GROUPS 
///////////////////////

#define LIGHTMAP_SIZE    4096
#define MAX_LIGHT_RADIUS 256
#define LIGHTS_PER_CELL  4
#define MAX_LIGHTS       (((LIGHTMAP_SIZE / MAX_LIGHT_RADIUS) * (LIGHTMAP_SIZE / MAX_LIGHT_RADIUS)) * LIGHTS_PER_CELL)

typedef struct point_light
{
    vec2_t  ws_position;
    vec2_t  direction;
    vec4_t  light_color;
    float32 radius;
    float32 cone_angle;
    float32 strength;
}point_light_t;

typedef struct shadow_caster2D
{
    render_quad_t quad_data;
    u32           light_atlas_cell_index;
    u32           light_atlas_channel_index;
}shadow_caster2D_t;

#define MAX_RENDER_GROUPS 96
#define MAX_RENDER_LAYERS 32
#define MAX_TEXTURES      16

typedef enum render_group_effects
{
    RGE_None     = 1 << 0,
    RGE_Lighting = 1 << 1,
    RGE_Count
}render_group_effects_t;

typedef enum render_group_desired_render_phase
{
    RGP_Invalid      = 1 << 0,
    RGP_MainGamePass = 1 << 1,
    RGP_PostBlitPass = 1 << 2,
}render_group_desired_render_phase_t;

typedef enum render_group_primitive_type
{
    RGPT_Invalid,
    RGPT_Quads,
    RGPT_Lines,
    RGPT_Count,
}render_group_primitive_type_t;

typedef struct render_group_desc
{
    GPU_shader_t                       *shader;
    render_group_effects_t              desired_effects;
    render_group_desired_render_phase_t desired_phase;
    render_group_primitive_type_t       primitive_type;
    u32                                 render_layer;

    mat4_t                              view_matrix;
    mat4_t                              projection_matrix;

    bool32                              supports_transparency;
}render_group_desc_t;

typedef struct render_group
{
    render_group_desc_t render_desc;

    render_quad_t      *quad_buffer;
    u32                 quad_count;

    render_line_t      *line_buffer;
    u32                 line_count;

    vertex_t           *vertex_buffer;
    u32                 vertex_count;

    u32 textureIDs[MAX_TEXTURES];
    u32 desired_texture_count;
    u32 GPU_bound_texture_counter;
}render_group_t;

typedef struct render_phase_data
{
    render_group_t  **opaque_render_groups;
    render_group_t  **transparent_render_groups;

    u32               opaque_render_group_counter;
    u32               transparent_render_group_counter;
}render_phase_data_t;

///////////////////////
// RENDER DATA
///////////////////////

typedef struct draw_frame
{
    bool8               is_initialized;

    render_phase_data_t preblit_pass_data;
    render_phase_data_t postblit_pass_data;

    render_group_t     *active_render_group;

    point_light_t      *point_lights;
    u32                 light_counter;

    shadow_caster2D_t  *shadow_casters;
    u32                 shadow_caster_counter;

    render_quad_t      *emmisive_quads;
    u32                 emmisive_quad_counter;
}draw_frame_t;

typedef struct render_state
{
    u32            backend_framebuffer_width;
    u32            backend_framebuffer_height;
    
    u32            primary_vao_id;
    u32            primary_vbo_id;
    u32            primary_ebo_id;

    GPU_shader_t   test_shader;
    GPU_shader_t   font_shader;

    memory_arena_t draw_frame_arena;
    draw_frame_t   draw_frame;

    struct
    {
        u32 ID;
        u32 color_attachment0;
        u32 color_attachment1;
        u32 depth_buffer;
    }primary_framebuffer;

    struct 
    {
        u32            framebuffer_id;
        u32            color_attachment0;

        render_quad_t *lighting_quads;
        u32            lighting_quad_counter;

        render_quad_t *shadow_quads;
        u32            shadow_quad_counter;

        GPU_shader_t   lighting_shader;
        mat4_t         projection_matrix;
    }lighting_data;
}render_state_t;

#if DEVELOPER_BUILD
    #define r_texture_delete(texture_view)                       asset_manager->gpu_data->r_texture_delete(texture_view)
    #define r_texture_make_gpu(texture, has_aa, sampling)        asset_manager->gpu_data->r_texture_make_gpu(texture, has_aa, sampling)
    #define r_texture_update_from_bitmap(asset_manager, texture) asset_manager->gpu_data->r_texture_update_from_bitmap(asset_manager, texture)
#else
    #define r_texture_delete(texture_view)                       r_texture_delete_(texture_view)
    #define r_texture_make_gpu(texture, has_aa, sampling)        r_texture_make_gpu_(texture, has_aa, sampling)
    #define r_texture_update_from_bitmap(asset_manager, texture) r_texture_update_from_bitmap_(asset_manager, texture)
#endif

typedef void r_texture_make_gpu_t(texture2D_t *texture, bool8 has_AA, filter_type_t filter_type);
typedef void r_texture_delete_t(texture_view_t *view);
typedef void r_texture_update_from_bitmap_t(asset_manager_t *asset_manager, texture2D_t *texture);

typedef struct GPU_functions
{
    r_texture_make_gpu_t           *r_texture_make_gpu;
    r_texture_delete_t             *r_texture_delete;
    r_texture_update_from_bitmap_t *r_texture_update_from_bitmap;
}GPU_functions_t;


////////////////////////////
// RENDER API FUNCTIONS
////////////////////////////
internal void r_texture_make_gpu_(texture2D_t *texture, bool8 has_AA, filter_type_t filter_type);
internal void r_texture_delete_(texture_view_t *view);
internal void r_texture_update_from_bitmap_(asset_manager_t *asset_manager, texture2D_t *texture);
/////////////////////////////


////////// JUNK MOVE THIS STUFF
internal inline void
c_radix_sort(void  *primary_buffer,
             void  *sorting_buffer,
             s32    item_count,
             usize  item_size,
             s32    value_offset,
             s32    bits_to_search)
{
    const s32 radix         = 256;
    const s32 bits_per_pass = 8;

    const s32 pass_count = (bits_to_search + bits_per_pass - 1) / bits_per_pass;
    const s64 half_range = 1ULL << (bits_to_search - 1);

    s64 count[radix];
    s64 digit_sum[radix];

    for(s32 pass_index = 0;
        pass_index < pass_count;
        ++pass_index)
    {
        s32 bit_shift = pass_index * bits_per_pass;
        
        memset(count, 0, sizeof(count));
        for(s32 item_index = 0;
            item_index < item_count;
            ++item_index)
        {
            u8 *item = (u8 *)primary_buffer + (item_index * item_size);
            u64 value_to_sort = *(u64 *)(item + value_offset);
            value_to_sort += half_range;

            u32 digit = (value_to_sort >> bit_shift) & (radix - 1);
            ++count[digit];
        }

        digit_sum[0] = 0;
        for(s32 sum_index = 1;
            sum_index < radix;
            ++sum_index)
        {
            digit_sum[sum_index] = digit_sum[sum_index - 1] + count[sum_index - 1];
        }

        for(s32 item_index = 0;
            item_index < item_count;
            ++item_index)
        {
            u8 *item = (u8 *)primary_buffer + (item_index * item_size);
            u64 value_to_sort = *(u64 *)(item + value_offset);
            value_to_sort += half_range;

            u32 digit = (value_to_sort >> bit_shift) & (radix - 1);
            memcpy((u8 *)sorting_buffer + (digit_sum[digit] * item_size), item, item_size);

            ++digit_sum[digit];
        }
    }

    memcpy(primary_buffer, sorting_buffer, item_count * item_size);
}

#endif
