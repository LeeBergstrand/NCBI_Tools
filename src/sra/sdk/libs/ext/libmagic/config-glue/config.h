#include <ncbiconf.h>

/* We only care about in-house platforms, and so hard-code the following
   additional settings. */

#define HAVE_DAYLIGHT 1
#define HAVE_LIMTS_H 1
#define HAVE_LIBZ 1
#define HAVE_MBRTOWC 1
#define HAVE_STAT_ST_RDEV 1
#define HAVE_STRERROR 1
#define HAVE_STRTOUL 1
#define HAVE_TM_ISDST 1
#define HAVE_WCTYPE_H 1
#define HAVE_WCWIDTH 1

#ifdef HAVE_STRUCT_TM_TM_ZONE
#  define HAVE_STRUCT_TM_TM_GMTOFF
#endif

#if defined(NCBI_OS_LINUX)  ||  defined(NCBI_OS_SOLARIS)
#  define BUILTIN_ELF 1
#endif

#ifdef NCBI_OS_LINUX
#  define MAJOR_IN_SYSMACROS 1
#endif

#ifdef NCBI_OS_SOLARIS
#  define MAJOR_IN_MKDEV 1
#endif

#ifndef NCBI_OS_MSWIN /* in case we decide to support Windows */
#  define HAVE_MKSTEMP 1
#  define HAVE_MMAP 1
#  define HAVE_STRTOF 1
#  define HAVE_SYS_MMAN_H 1
#  define HAVE_SYS_PARAM 1
#  define HAVE_SYS_WAIT_H 1
#  define HAVE_UTIME 1
#endif
