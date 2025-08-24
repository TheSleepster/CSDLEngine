#if !defined(A_ASSET_LOADED_SOUND_H)
/* ========================================================================
   $File: a_asset_loaded_sound.h $
   $Date: Sun, 24 Aug 25: 10:48AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define A_ASSET_LOADED_SOUND_H
#include "c_base.h"
#include "c_types.h"
#include "c_math.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_string.h"
#include "c_array.h"
#include "c_file_api.h"

typedef struct loaded_sound
{
    s16 *sample_data;
    s32  sample_count;

    s32  channel_count;
}loaded_sound_t;

#endif
