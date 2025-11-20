/* ========================================================================
   $File: r_render_material.cpp $
   $Date: November 19 2025 08:08 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "r_render_material.h"

internal render_material_t
r_render_material_create(texture2D_t *texture, GPU_shader_t *shader)
{
    render_material_t result;

    result.texture     = texture;
    result.shader      = shader;

    // NOTE(Sleepster): This is potentially volatile if the memory address is different, but the shader/texture itself is the the same
    // good enough for now though since everything is stored within the asset_manager.
    uint64_t A = (uint64_t)texture;
    uint64_t B = (uint64_t)shader;
    A ^= B + 0x9e3779b97f4a7c15ULL + (A << 6) + (A >> 2);

    result.material_ID = A;

    return(result);
}
