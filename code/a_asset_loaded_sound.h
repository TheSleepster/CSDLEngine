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

typedef struct asset_slot    asset_slot_t;
typedef struct asset_handle  asset_handle_t;
typedef struct asset_manager asset_manager_t;

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24)) 
enum 
{
    WAVE_chunkID_fmt  = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_chunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_chunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
    WAVE_chunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
};

#pragma pack(push, 1)
typedef struct WAVE_header
{
    u32 RIFFID;
    u32 Size;
    u32 WAVEID;
}WAVE_header_t;

typedef struct WAVE_chunk
{
    u32 ID;
    u32 size;
}WAVE_chunk_t;

typedef struct WAVE_format_data
{
    u16 wFormatTag;
    u16 nChannels;
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    u16 nBlockAlign;
    u16 wBitsPerSample;
    u16 cbSize;
    u16 wValidBitsPerSample;
    u32 dwChannelMask;
    u8  SubFormat[16];
}WAVE_format_data_t;
#pragma pack(pop)

typedef struct WAVE_file_iterator
{
    uint8      *at;
    uint8      *end;
}WAVE_file_iterator_t;

typedef struct loaded_sound
{
    s16 *sample_data;
    s32  sample_count;

    s32  channel_count;
}loaded_sound_t;

#endif
