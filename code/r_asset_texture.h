#if !defined(R_ASSET_TEXTURE_H)
/* ========================================================================
   $File: r_asset_texture.h $
   $Date: Sat, 02 Aug 25: 12:40AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_ASSET_TEXTURE_H
#include "c_types.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_file_api.h"

typedef enum bitmap_format
{
    BMF_R8     = 1,
    BMF_B8     = 1,
    BMF_G8     = 1,
    BMF_RGB24  = 3,
    BMF_BGR24  = 3,
    BMF_RGBA32 = 4,
    BMF_ABGR32 = 4,
    BMF_COUNT
}bitmap_format_t;

typedef struct bitmap
{
    bitmap_format_t format;
    s32             channels;
    s32             width;
    s32             height;
    s32             stride;
    
    u8             *data;
}bitmap_t;

typedef enum filter_type
{
    TAAFT_INVALID,
    TAAFT_LINEAR,
    TAAFT_NEAREST,
    TAAFT_COUNT
}filter_type_t;

typedef struct texture2D
{
    u32           gpu_texture_ID;
    bitmap_t      bitmap;

    bool8         has_AA;
    filter_type_t filter_type;
}texture2D_t;

typedef struct sprite2D
{
    ivec2_t offset;
    ivec2_t size;
}sprite2D_t;

#endif
