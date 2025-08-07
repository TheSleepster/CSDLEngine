#if !defined(C_BASE_H)
/* ========================================================================
   $File: c_base.h $
   $Date: Mon, 21 Jul 25: 09:42AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_BASE_H

// MSVC
#if defined(_MSC_VER)
#define COMPILER_CL 1
#if defined(_WIN_32)
#define OS_WINDOWS 1
#endif

// ARCH
#if defined(_M_AMD64)
# define ARCH_X64 1
#elif defined(_M_I86)
# define ARCH_X86 1
#elif defined(_M_ARM)
# define ARCH_ARM 1
#else
# error ARCH not found...
#endif

// CLANG
#elif defined(__clang__)
# define COMPILER_CLANG 1
#if defined(_WIN_32)
# define OS_WINDOWS 1
#elif defined(__gnu_linux__)
# define OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
# define OS_MAC 1
#else
# error OS not detected...
#endif

// ARCH
#if defined(__amd64__)
# define ARCH_X64 1
#  elif defined(__i386__)
# define ARCH_X86 1
#  elif defined(__arm__)
# define ARCH_ARM 1
#  elif defined(__aarch_64__)
# define ARCH_ARM64 1
#else
# error x86/x64 is the only supported architecture at the moment...
#endif

// GNU C
#elif defined(__GNUC__)
# define COMPILER_GCC 1
#if defined(_WIN_32)
# define OS_WINDOWS 1
#elif defined(__gnu_linux__)
# define OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
# define OS_MAC 1
#else
# error OS not detected...
#endif
    
// ARCH
#if defined(__amd64__)
# define ARCH_X64 1
#elif defined(__i386__)
# define ARCH_X86 1
#elif defined(__arm__)
# define ARCH_ARM 1
#elif defined(__aarch_64__)
# define ARCH_ARM64 1
#else
# error cannot find ARCH...
#endif
#else
# error unable to distinguish this compiler...
#endif

// COMPILERS
#if !defined(COMPILER_CL)
# define COMPILER_CL 0
#endif
#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif

// OPERATING SYSTEMS
#if !defined(OS_WINDOWS)
# define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
# define OS_LINUX 0
#endif
#if !defined(OS_MAC)
# define OS_MAC 0
#endif

// ARCH
#if !defined(ARCH_X64)
# define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
# define ARCH_X86 0
#endif
#if !defined(ARCH_ARM)
# define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
# define ARCH_ARM64 0
#endif

#if !defined(ASSERT_ENABLED)
# define ASSERT_ENABLED
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

///////////////////////////////////
// NOTE(Sleepster): HELPER MACROS 
#define Min(a, b) (((a) > (b)) ? (b) : (a))
#define Max(a, b) (((a) < (b)) ? (b) : (a))

#define Statement(x)                 do{x}while(0)

#define GlueHelper(A, B)             A##B
#define Glue(A, B)                   GlueHelper(A, B)

#define ArrayCount(x)                (sizeof(x) / sizeof(*(x)))
#define IntFromPtr(x)                ((u32)    ((char *)x - (char*)0))
#define PtrFromInt(x)                ((void *) ((char *)0 + (x)))

#define MemberHelper(type, member)   (((type *)0)->member)
#define OffsetOf(type, member)       (&MemberHelper(type, member))
#define OffsetOfInt(type, member)    (IntFromPtr(OffsetOf(type, member)))

#include <string.h>
#define ZeroMemory(data, size)       (memset((data), 0, (size)))
#define ZeroStruct(type)             (ZeroMemory(&type, sizeof(type)))

#define MemoryCopy(dest, source, size) (memmove((dest), (source), (size)))
#define MemoryCopyStruct(dest, source) (MemoryCopy((dest), (source), Min(sizeof(*(dest), sizeof(*(source))))))

#define InvalidCodePath Assert(false)

// NOTE(Sleepster): Linux Kernel compile time assert! It unfortunately only tells you if the the condition fails... not how it fails... Unlucky!
#define StaticAssert(condition) ((void)sizeof(char[2*!!(condition) - 1]))

#include "c_types.h"

global s8  min_s8  = (s8) 0x80;
global s16 min_s16 = (s16)0x8000;
global s32 min_s32 = (s32)0x80000000;
global s64 min_s64 = (s64)0x8000000000000000llu;

global s8  max_s8  = (s8) 0x7f;
global s16 max_s16 = (s16)0x7ffff;
global s32 max_s32 = (s32)0x7ffffffff;
global s64 max_s64 = (s64)0x7fffffffffffffffllu;

global u8  min_u8  = (u8) 0x80;
global u16 min_u16 = (u16)0x8000;
global u32 min_u32 = (u32)0x80000000;
global u64 min_u64 = (u64)0x8000000000000000llu;

global u8  max_u8  = (u8) 0x7f;
global u16 max_u16 = (u16)0x7ffff;
global u32 max_u32 = (u32)0x7fffffff;
global u64 max_u64 = (u64)0x7fffffffffffffffllu;

global float32 machine_epsilon_f32 = 1.1920929e-7f;
global float64 machine_epsilon_f64 = 2.220446e-16;

typedef void void_func(void);

typedef enum operating_system
{
    OS_Null,
    OS_Windows,
    OS_Linux,
    OS_MacOSX,
    OS_Web,
    OS_COUNT
}operating_system_t;

typedef enum system_architecture
{
    SA_Null,
    SA_X64,
    SA_X86,
    SA_Arm,
    SA_ArmM64,
    SA_COUNT
}system_architecture_t;

#include "c_memory.h"
#include "c_string.h"
#include "c_array.h"

// USE ARENAS NOT MALLOC
typedef struct global_context
{
    memory_arena_t context_arena;
    memory_arena_t temporary_arena;
}global_context_t;

global global_context_t global_context;

internal inline void
gc_setup()
{
    global_context.context_arena   = c_arena_create(MB(100));
    global_context.temporary_arena = c_arena_create(MB(10));
}

internal inline void
gc_reset_temporary_data()
{
    c_arena_reset(&global_context.temporary_arena);
}

#endif
