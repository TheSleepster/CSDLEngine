#if !defined(OS_PLATFORM_FILE_H)
/* ========================================================================
   $File: os_platform_file.h $
   $Date: Mon, 28 Jul 25: 07:44PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define OS_PLATFORM_FILE_H
#include "c_base.h"
#include "c_types.h"

#if OS_WINDOWS
# include "os_windows.c"

#elif OS_LINUX
# include "os_linux.c"

#elif OS_MAC
# error "lmao really?"
#endif

#endif
