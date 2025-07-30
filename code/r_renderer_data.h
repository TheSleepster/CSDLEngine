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
#define MAX_INDICES  (MAX_QUADS * 6)

#include "c_types.h"
#include "c_math.h"

////////////////////////////////////////
// RENDERING PRIMITIVES 

typedef struct vertex
{
    vec2_t vPosition;
    vec4_t vColor;
}vertex_t;

typedef enum render_quad_options
{
    RQO_None         = 0x00,
    RQO_Unlit        = 0x01,
    RQO_ShadowCaster = 0x02,
    RQO_Count,
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
    u32                   texture_id;
    render_quad_options_t options;
}render_quad_t;


////////////////////////////////
// SHADER DATA

#define UPDATE_SHADER_UNIFORM(name) void name(s32 location_id, void *data)
typedef UPDATE_SHADER_UNIFORM(update_shader_uniform_t);

#define UPDATE_GPU_SHADER_STORAGE_BUFFER_OBJECT(name) void name(s32 location_id, s64 size, void *data)
typedef UPDATE_GPU_SHADER_STORAGE_BUFFER_OBJECT(update_shader_ssbo_t);

#define UPDATE_GPU_UNIFORM_BUFFER_OBJECT(name) void name(s32 location_id, s64 size, void *data)
typedef UPDATE_GPU_UNIFORM_BUFFER_OBJECT(update_shader_ubo_t);

typedef struct shader_storage_buffer
{
    s32                   location_id;
    string_t              name;
    u32                   binding;
    bool8                 is_read_only;

    void                 *data;
    s64                   size;
    s64                   old_size;

    update_shader_ssbo_t *update;
}shader_storage_buffer_t;

typedef enum shader_uniform_type
{
    SUT_INVALID,
    SUT_INTEGER,
    SUT_UNSIGNED_INTEGER,
    SUT_FLOAT,
    SUT_FLOAT_VECTOR2,
    SUT_FLOAT_VECTOR3,
    SUT_FLOAT_VECTOR4,
    SUT_FLOAT_MATRIX3,
    SUT_FLOAT_MATRIX4,
    SUT_TEXTURE_BINDING,
    SUT_TEXTURE_ARRAY,
}shader_uniform_type_t;

typedef struct shader_uniform
{
    s32                      location_id;
    shader_uniform_type_t    type;
    string_t                 name;

    void                    *data;
    s32                      size;

    update_shader_uniform_t *update;
}shader_uniform_t;

typedef enum gpu_shader_type
{
    ST_PIXEL_SHADER   = 0,
    ST_COMPUTE_SHADER = 1,
    ST_COUNT,
}gpu_shader_type_t;

typedef struct GPU_shader
{
    s32      program_id;
    string_t shader_source;

    array_t  shader_uniforms;
    array_t  shader_uniform_buffers;
    array_t  shader_storage_buffers;
}GPU_shader_t;

internal void r_update_shader_SSBO_data   (GPU_shader_t *shader, string_t name, void *data, s64 size);
internal void r_update_shader_UBO_data    (GPU_shader_t *shader, string_t name, void *data);
internal void r_update_shader_uniform_data(GPU_shader_t *shader, string_t name, void *data);

///////////////////////
// RENDER GROUPS 

#define MAX_RENDER_GROUPS 96
#define MAX_RENDER_LAYERS 32

typedef struct render_layer_data
{
    u32 layer_pass_mask;

    u32 layer_passes[MAX_RENDER_LAYERS];
    u32 layer_pass_count;
}render_layer_data_t;

typedef enum render_group_effects
{
    RGE_None     = 0x00,
    RGE_Lighting = 0x01,
    RGE_Bloom    = 0x02,
    RGE_Count
}render_group_effects_t;

typedef struct render_group_desc
{
    GPU_shader_t          *shader;
    render_group_effects_t render_effects;
    u32                    render_layer;
    u32                    layer_priority;

    mat4_t                 view_matrix;
    mat4_t                 projection_matrix;
}render_group_desc_t;

typedef struct render_group
{
    render_group_desc_t render_desc;

    vertex_t           *vertex_buffer;
    u32                 vertex_count;

    render_quad_t      *buffer_quads;
    u32                 quad_count;
}render_group_t;


///////////////////////
// RENDER DATA

#define LIGHTMAP_SIZE 4096
#define MAX_LIGHTS   (4096.0f/256.0f)

typedef struct point_light
{
    vec2_t ws_position;
    vec4_t light_color;
}point_light_t;

typedef struct draw_frame
{
    bool8                 initialized;
    
    render_group_t      **render_groups;
    render_group_t       *active_render_group;
    u32                   render_group_counter;

    // NOTE(Sleepster): bitmask 
    u32                   render_layer_mask;
    u32                   used_render_layer_count;
    u32                   used_render_layers[MAX_RENDER_LAYERS];
    render_layer_data_t   render_layer_data [MAX_RENDER_LAYERS];

    point_light_t        *point_lights;
    u32                   light_counter;
}draw_frame_t;

typedef struct render_state
{
    u32                     primary_vao_id;
    u32                     primary_vbo_id;
    u32                     primary_ebo_id;

    GPU_shader_t            test_shader;

    memory_arena_t          draw_frame_arena;
    draw_frame_t            draw_frame;

    struct 
    {
        u32            framebuffer_id;
        u32            color_attachment_0;

        render_quad_t *lighting_quads;
        u32            lighting_quad_counter;

        render_quad_t *shadow_quads;
        u32            shadow_quad_counter;

        GPU_shader_t  *lighting_shader;
        mat4_t         projection_matrix;
    }lighting_data;
}render_state_t;

////////// JUNK MOVE THIS STUFF
internal inline u64
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
