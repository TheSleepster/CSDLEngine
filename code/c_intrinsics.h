#if !defined(C_INTRINSICS_H)
/* ========================================================================
   $File: c_intrinsics.h $
   $Date: Tue, 29 Jul 25: 05:18PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_INTRINSICS_H

#if defined COMPILER_CLANG || defined COMPILER_GCC
#define popcount32(value) __builtin_popcount(value)

#elif defined COMPILER_CL
#endif

#endif
