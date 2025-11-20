#if !defined(R_RENDER_MATERIAL_H)
/* ========================================================================
   $File: r_render_material.h $
   $Date: November 19 2025 08:12 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_RENDER_MATERIAL_H
#include "r_asset_texture.h"
#include "r_asset_shader.h"

// NOTE(Sleepster): This is a collection of data used to help with identifying certain render_groups
typedef struct texture2D texture2D_t;
typedef struct render_material
{
    // NOTE(Sleepster): name is optional 
    string_t        name;
    u32             material_ID;
    u32             render_effect_mask;

    texture2D_t    *texture;
    GPU_shader_t   *shader;
}render_material_t;

internal render_material_t r_render_material_create(texture2D_t *texture, GPU_shader_t *shader);

#endif // R_RENDER_MATERIAL_H

