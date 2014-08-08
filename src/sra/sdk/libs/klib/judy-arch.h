/* $Id */

#include <ncbiconf.h>
#if NCBI_PLATFORM_BITS == 64
#  define JU_64BIT
#else
#  define JU_32BIT
#endif
#ifdef NCBI_OS_MSWIN
/* #  define JU_WIN */
#  define JU_WMAIN
#endif
