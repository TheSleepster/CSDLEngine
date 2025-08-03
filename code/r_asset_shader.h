#if !defined(R_ASSET_SHADER_H)
/* ========================================================================
   $File: r_asset_shader.h $
   $Date: Sat, 02 Aug 25: 12:06AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_ASSET_SHADER_H
////////////////////////////////
// SHADER DATA
#include "c_types.h"
#include "c_string.h"
#include "c_array.h"

#define UPDATE_SHADER_UNIFORM(name) void name(s32 location_id, void *data)
typedef UPDATE_SHADER_UNIFORM(update_shader_uniform_t);

#define UPDATE_GPU_SHADER_STORAGE_BUFFER_OBJECT(name) void name(s32 location_id, s64 size, void *data)
typedef UPDATE_GPU_SHADER_STORAGE_BUFFER_OBJECT(update_shader_ssbo_t);

#define UPDATE_GPU_UNIFORM_BUFFER_OBJECT(name) void name(s32 location_id, s64 size, void *data)
typedef UPDATE_GPU_UNIFORM_BUFFER_OBJECT(update_shader_ubo_t);

internal void r_resize_SSBO(s32 location_id, s64 new_size);

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

#endif
