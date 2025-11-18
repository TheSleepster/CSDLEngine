#if !defined(RENDER_DATA_H)
/* ========================================================================
   $File: render_data.h $
   $Date: November 17 2025 02:34 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define RENDER_DATA_H

#define MAX_QUADS     (2500)
#define MAX_VERTICES  (MAX_QUADS * 4)
#define MAX_LINES     (MAX_VERTICES / 2)
#define MAX_INDICES   (MAX_QUADS * 6)
#define MAX_MATERIALS (1024)

#define LIGHTMAP_SIZE    4096
#define MAX_LIGHT_RADIUS 256
#define LIGHTS_PER_CELL  4
#define MAX_LIGHTS       (((LIGHTMAP_SIZE / MAX_LIGHT_RADIUS) * (LIGHTMAP_SIZE / MAX_LIGHT_RADIUS)) * LIGHTS_PER_CELL)

#define RENDER_GROUP_HASH_COUNT     512
#define MAX_RENDER_GROUPS_TO_OUTPUT 64
#define MAX_RENDER_LAYERS           32
#define MAX_TEXTURES                16

#include "s_asset_manager.h"
#include "r_asset_texture.h"

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

typedef struct render_camera
{
    mat4 view_matrix;
    mat4 projection_matrix;
}render_camera_t;

// NOTE(Sleepster): This is a collection of data used to help with identifying certain render_groups
typedef struct texture2D texture2D_t;
typedef struct render_material
{
    // NOTE(Sleepster): name is optional 
    string_t       name;

    u32            material_ID;
    u32            render_effect_mask;

    texture2D_t   *texture;
    GPU_shader_t  *shader;
}render_material_t;

typedef enum render_group_primitive_type
{
    RGPT_Invalid,
    RGPT_Quads,
    RGPT_Lines,
    RGPT_Count,
}render_group_primitive_type_t;

typedef enum render_phase 
{
    RGP_Invalid  = 1 << 0,
    RGP_Preblit  = 1 << 1,
    RGP_Postblit = 1 << 2
}render_phase_t;

typedef enum render_group_effects
{
    RGE_None     = 1 << 0,
    RGE_Lighting = 1 << 1,
    RGE_Count
}render_group_effects_t;

typedef struct render_backend_data render_backend_data_t;
typedef enum render_group_blending_mode
{
    RGBM_Invalid,
    RGBM_Zero,
    RGBM_One,
    RGBM_Constant,

    RGBM_SrcColor,
    RGBM_OneMinusSrcColor,
    RGBM_DstColor,
    RGBM_OneMinusDstColor,

    RGBM_SrcAlpha,
    RGBM_OneMinusSrcAlpha,
    RGBM_DstAlpha,
    RGBM_OneMinusDstAlpha,
    RGBM_Count
}render_group_blending_mode_t;

typedef enum render_group_blending_equation
{
    RGBE_Invalid,
    RGBE_Add,
    RGBE_Subtract,
    RGBE_ReverseSubtract,
    RGBE_Min,
    RGBE_Max,
}render_group_blending_equation_t;

typedef enum render_group_depth_function
{
    RGDF_Invalid,
    RGDF_Never,
    RGDF_Always,

    RGDF_Greater,
    RGDF_Less,
    RGDF_Equal,
    RGDF_NotEqual,
    RGDF_LessOrEqual,
    RGDF_GreaterOrEqual,
    RGDF_Count
}render_group_depth_function_t;

typedef struct render_pipeline_state
{
    render_group_blending_mode_t     src_color_blend_mode;
    render_group_blending_mode_t     dst_color_blend_mode;
    render_group_blending_equation_t color_blend_eq;

    render_group_blending_mode_t     src_alpha_blend_mode;
    render_group_blending_mode_t     dst_alpha_blend_mode;
    render_group_blending_equation_t alpha_blend_eq;
    bool8                            blending;

    render_group_depth_function_t    depth_func;
    bool8                            depth_testing;
    bool8                            depth_writing;

    float32                          render_line_width;

    bool8                            scissor_enabled;
    u32                              scissor_x;
    u32                              scissor_y;
    u32                              scissor_w;
    u32                              scissor_h;
}render_pipeline_state_t;

#include "r_render_group.h"

typedef struct render_group_container
{
    hash_table_t group_hash;
    u32          render_group_ids[MAX_RENDER_GROUPS_TO_OUTPUT];
    u32          used_render_group_counter;
}render_group_container_t;

typedef struct render_phase_data
{
    render_group_container_t opaque;
    render_group_container_t transparent;
}render_phase_data_t;

typedef struct draw_frame_data
{
    u32               shadow_caster_count;
    u32               light_count;

    GPU_shader_t     *active_shader;
    render_camera_t   active_camera;
    render_material_t active_material;
    render_phase_t    active_render_phase;
    u32               active_render_layer;

    render_group_t   *active_render_group;
}draw_frame_data_t;

typedef struct render_state
{
    memory_arena_t          persistant_arena;
    memory_arena_t          frame_arena;

    u32                     framebuffer_width;
    u32                     framebuffer_height; 

    render_backend_data_t  *backend;
    render_pipeline_state_t pipeline_state;

    draw_frame_data_t       draw_frame;

    render_phase_data_t     preblit_phase;
    render_phase_data_t     postblit_phase;

    // TODO(Sleepster): TEMPORARY 
    GPU_shader_t            font_shader;
    GPU_shader_t            lighting_shader;
    GPU_shader_t            test_shader;
}render_state_t;

internal void r_init_renderer_data(SDL_Window *window, render_state_t *render_state);
internal void r_render_single_frame(render_state_t *render_state, asset_manager_t *asset_manager);

// NOTE(Sleepster): This is mainly used for DLL_RELOADING 
#if DLL_RELOADING 
// TODO(Sleepster): rename to r_texture_upload()
    void r_texture_make_gpu_(texture2D_t *texture, bool8 has_AA, filter_type_t filter_type);
    void r_texture_delete_(texture_view_t *view);
    void r_texture_update_from_bitmap_(asset_manager_t *asset_manager, texture2D_t *texture);

    #define r_texture_delete(texture_view)                       asset_manager->gpu_data->r_texture_delete(texture_view)
    #define r_texture_make_gpu(texture, has_aa, sampling)        asset_manager->gpu_data->r_texture_make_gpu(texture, has_aa, sampling)
    #define r_texture_update_from_bitmap(asset_manager, texture) asset_manager->gpu_data->r_texture_update_from_bitmap(asset_manager, texture)
#else
    void r_texture_make_gpu_(texture2D_t *texture, bool8 has_AA, filter_type_t filter_type);
    void r_texture_delete_(texture_view_t *view);
    void r_texture_update_from_bitmap_(asset_manager_t *asset_manager, texture2D_t *texture);

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

internal render_quad_t*
r_draw_rect(render_state_t       *render_state,
            vec2_t                position,
            vec2_t                render_size,
            vec4_t                color,
            float32               rotation,
            render_quad_options_t render_options);


#endif // RENDER_DATA_H

