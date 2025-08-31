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
  - MSVC native srinsics (????)
  - ARM NEON srinsicS
 */

#if defined COMPILER_CLANG || defined COMPILER_GCC

#if ARCH_X64
    #include <emmintrin.h>
    #include <xmmintrin.h>

    #define popcount32(value) __builtin_popcount(value)

 /* ===========================================
    ================== FENCES =================
    ===========================================*/
    #define c_atomic_mfence() __atomic_thread_fence(__ATOMIC_SEQ_CST)
    #define c_atomic_sfence() __atomic_thread_fence(__ATOMIC_RELEASE)
    #define c_atomic_lfence() __atomic_thread_fence(__ATOMIC_ACQUIRE)

    #define c_mm_mfence() _mm_mfence()
    #define c_mm_sfence() _mm_sfence()
    #define c_mm_lfence() _mm_lfence()

 /* =============================================================
    ====================== ATOMIC INCREMENT =====================
    ============================================================= */
    #define c_atomic_increment16(ptr) __atomic_add_fetch((s16*)(ptr), 1, __ATOMIC_SEQ_CST)
    #define c_atomic_increment32(ptr) __atomic_add_fetch((s32*)(ptr), 1, __ATOMIC_SEQ_CST)
    #define c_atomic_increment64(ptr) __atomic_add_fetch((s64*)(ptr), 1, __ATOMIC_SEQ_CST)

 /* =============================================================
    ====================== ATOMIC DECREMENT =====================
    ============================================================= */
    #define c_atomic_decrement16(ptr) __atomic_sub_fetch((s16*)(ptr), 1, __ATOMIC_SEQ_CST)
    #define c_atomic_decrement32(ptr) __atomic_sub_fetch((s32*)(ptr), 1, __ATOMIC_SEQ_CST)
    #define c_atomic_decrement64(ptr) __atomic_sub_fetch((s64*)(ptr), 1, __ATOMIC_SEQ_CST)

 /* =============================================================
    ======================== ATOMIC ADD =========================
    ============================================================= */
    #define c_atomic_add16(ptr, value) __atomic_fetch_add((s16*)(ptr), (s16)value, __ATOMIC_SEQ_CST)
    #define c_atomic_add32(ptr, value) __atomic_fetch_add((s32*)(ptr), (s32)value, __ATOMIC_SEQ_CST)
    #define c_atomic_add64(ptr, value) __atomic_fetch_add((s64*)(ptr), (s64)value, __ATOMIC_SEQ_CST)

 /* =============================================================
    ====================== ATOMIC SUBTRACT ======================
    ============================================================= */
    #define c_atomic_sub16(ptr, value) __atomic_fetch_sub((s16*)(ptr), (s16)value, __ATOMIC_SEQ_CST)
    #define c_atomic_sub32(ptr, value) __atomic_fetch_sub((s32*)(ptr), (s32)value, __ATOMIC_SEQ_CST)
    #define c_atomic_sub64(ptr, value) __atomic_fetch_sub((s64*)(ptr), (s64)value, __ATOMIC_SEQ_CST)

 /* =============================================================
    ====================== ATOMIC EXCHANGE ======================
    ============================================================= */
    #define c_atomic_exchange16(ptr, val) __atomic_exchange_n((s16*)(ptr), (s16)(val), __ATOMIC_SEQ_CST)
    #define c_atomic_exchange32(ptr, val) __atomic_exchange_n((s32*)(ptr), (s32)(val), __ATOMIC_SEQ_CST)
    #define c_atomic_exchange64(ptr, val) __atomic_exchange_n((s64*)(ptr), (s64)(val), __ATOMIC_SEQ_CST)

 /* =============================================================
    =================== ATOMIC COMPARE EXCHANGE =================
    ============================================================= */
    #define c_atomic_compare_exchange16(ptr, exchange, comparand) ({    \
        s16 _expected = (int16_t)(comparand    );                       \
        __atomic_compare_exchange_n((s16*)(ptr), &_expected,            \
                                    (s16)(exchange), 0,                 \
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
        _expected;                                                      \
    })

    #define c_atomic_compare_exchange32(ptr, exchange, comparand) ({    \
        s32 _expected = (int32_t)(comparand);                           \
        __atomic_compare_exchange_n((s32*)(ptr), &_expected,            \
                                    (s32)(exchange), 0,                 \
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
        _expected;                                                      \
    })

    #define c_atomic_compare_exchange64(ptr, exchange, comparand) ({    \
        int64_t _expected = (s64)(comparand);                           \
        __atomic_compare_exchange_n((s64*)(ptr), &_expected,            \
                                    (s64)(exchange), 0,                 \
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
        _expected;                                                      \
    })
#endif

#elif defined COMPILER_CL
#endif

#endif
