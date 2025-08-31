#if !defined(C_SRINSICS_H)
/* ========================================================================
   $File: c_srinsics.h $
   $Date: Tue, 29 Jul 25: 05:18PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_SRINSICS_H

#include "c_types.h"

/*
  TODO:
  - MSVC native intrinsics (????)
  - ARM NEON intrinsics
 */

#if defined COMPILER_CLANG || defined COMPILER_GCC
    #if ARCH_X64
        #include <emmintrin.h>
        #include <xmmintrin.h>

        #define PopCount32(value) __builtin_popcount(value)

    /* ===========================================
       ================== FENCES =================
       ===========================================*/
        #define mfence() __atomic_thread_fence(__ATOMIC_SEQ_CST)
        #define sfence() __atomic_thread_fence(__ATOMIC_RELEASE)
        #define lfence() __atomic_thread_fence(__ATOMIC_ACQUIRE)

        // NOTE(Sleepster): These are COMPILER barriers. "Don't move reads/writes above this line" 
        #define ReadBarrier       __asm__ __volatile__("" ::: "memory")
        #define WriteBarrier      __asm__ __volatile__("" ::: "memory")
        #define ReadWriteBarrier  __asm__ __volatile__("" ::: "memory")

    /* =============================================================
       ====================== ATOMIC INCREMENT =====================
       ============================================================= */
        #define AtomicIncrement16(ptr) __atomic_add_fetch((s16*)(ptr), 1, __ATOMIC_SEQ_CST)
        #define AtomicIncrement32(ptr) __atomic_add_fetch((s32*)(ptr), 1, __ATOMIC_SEQ_CST)
        #define AtomicIncrement64(ptr) __atomic_add_fetch((s64*)(ptr), 1, __ATOMIC_SEQ_CST)

        #define AtomicIncrement(ptr) AtomicIncrement32(ptr)

    /* =============================================================
       ====================== ATOMIC DECREMENT =====================
       ============================================================= */
        #define AtomicDecrement16(ptr) __atomic_sub_fetch((s16*)(ptr), 1, __ATOMIC_SEQ_CST)
        #define AtomicDecrement32(ptr) __atomic_sub_fetch((s32*)(ptr), 1, __ATOMIC_SEQ_CST)
        #define AtomicDecrement64(ptr) __atomic_sub_fetch((s64*)(ptr), 1, __ATOMIC_SEQ_CST)

        #define AtomicDecrement(ptr) AtomicDecrement32(ptr)

    /* =============================================================
       ======================== ATOMIC ADD =========================
       ============================================================= */
        #define AtomicAdd16(ptr, value) __atomic_add_fetch((s16*)(ptr), (s16)value, __ATOMIC_SEQ_CST)
        #define AtomicAdd32(ptr, value) __atomic_add_fetch((s32*)(ptr), (s32)value, __ATOMIC_SEQ_CST)
        #define Atomicadd64(ptr, value) __atomic_add_fetch((s64*)(ptr), (s64)value, __ATOMIC_SEQ_CST)

        #define AtomicAdd(ptr, value) AtomicAdd32(ptr, value)

    /* =============================================================
       =================== ATOMIC ADD EXCHANGE ====================
       ============================================================= */
        #define AtomicExchangeAdd16(ptr, value) __atomic_fetch_add((s16*)(ptr), (s16)value, __ATOMIC_SEQ_CST)
        #define AtomicExchangeAdd32(ptr, value) __atomic_fetch_add((s32*)(ptr), (s32)value, __ATOMIC_SEQ_CST)
        #define AtomicExchangeAdd64(ptr, value) __atomic_fetch_add((s64*)(ptr), (s64)value, __ATOMIC_SEQ_CST)

        #define AtomicExchangeAdd(ptr, value) AtomicAdd32(ptr, value)

    /* =============================================================
       ====================== ATOMIC SUBTRACT ======================
       ============================================================= */

        // NOTE(Sleepster): Technically not a real thing intrinsic but convienence is king.
        #define AtomicSubtract16(ptr, value) __atomic_add_fetch((s16*)(ptr), (s16)-value, __ATOMIC_SEQ_CST)
        #define AtomicSubtract32(ptr, value) __atomic_add_fetch((s32*)(ptr), (s32)-value, __ATOMIC_SEQ_CST)
        #define AtomicSubtract64(ptr, value) __atomic_add_fetch((s64*)(ptr), (s64)-value, __ATOMIC_SEQ_CST)

        #define AtomicSubtract(ptr, value) AtomicSubtract32(ptr, val) 

    /* =============================================================
       ================= ATOMIC SUBTRACT EXCHANGE ==================
       ============================================================= */
        #define AtomicExchangeSubtract16(ptr, value) __atomic_fetch_add((s16*)(ptr), (s16)-value, __ATOMIC_SEQ_CST)
        #define AtomicExchangeSubtract32(ptr, value) __atomic_fetch_add((s32*)(ptr), (s32)-value, __ATOMIC_SEQ_CST)
        #define AtomicExchangeSubtract64(ptr, value) __atomic_fetch_add((s64*)(ptr), (s64)-value, __ATOMIC_SEQ_CST)

        #define AtomicExchangeSubtract(ptr, value) AtomicExchangeSubtract32(ptr, val) 

    /* =============================================================
       ====================== ATOMIC EXCHANGE ======================
       ============================================================= */
        #define AtomicExchange16(ptr, val) __atomic_exchange_n((s16*)(ptr), (s16)(val), __ATOMIC_SEQ_CST)
        #define AtomicExchange32(ptr, val) __atomic_exchange_n((s32*)(ptr), (s32)(val), __ATOMIC_SEQ_CST)
        #define AtomicExchange64(ptr, val) __atomic_exchange_n((s64*)(ptr), (s64)(val), __ATOMIC_SEQ_CST)

        #define AtomicExchange(ptr, val) AtomicExchange32(ptr, val)

    /* =============================================================
       =================== ATOMIC COMPARE EXCHANGE =================
       ============================================================= */
        #define AtomicCompareExchange16(ptr, exchange, comparand) ({        \
            s16 _expected = (int16_t)(comparand    );                       \
            __atomic_compare_exchange_n((s16*)(ptr), &_expected,            \
                                        (s16)(exchange), 0,                 \
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
            _expected;                                                      \
        })

        #define AtomicCompareExchange32(ptr, exchange, comparand) ({        \
            s32 _expected = (int32_t)(comparand);                           \
            __atomic_compare_exchange_n((s32*)(ptr), &_expected,            \
                                        (s32)(exchange), 0,                 \
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
            _expected;                                                      \
        })

        #define AtomicCompareExchange64(ptr, exchange, comparand) ({        \
            int64_t _expected = (s64)(comparand);                           \
            __atomic_compare_exchange_n((s64*)(ptr), &_expected,            \
                                        (s64)(exchange), 0,                 \
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
            _expected;                                                      \
        })

        #define AtomicCompareExchange(ptr, exchange, comparand) AtomicCompareExchange32(ptr, exchange, comparand)
    #else
        #error "only x64 is supported..."
    #endif
#else
    #error "Only GCC/Clang are supported..."
#endif

#endif
