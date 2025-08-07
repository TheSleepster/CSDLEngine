#if !defined(C_TYPES_H)
/* ========================================================================
   $File: c_types.h $
   $Date: Wed, 02 Jul 25: 06:11PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_TYPES_H
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t   usize;
typedef u8       byte;

typedef u8       bool8;
typedef u32      bool32;

typedef float    float32;
typedef double   float64;

typedef float    real32;
typedef double   real64;

#define null (void*)0 

#define global        static
#define internal      static
#define local_persist static

#endif
