/* $Id: os-native.h 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
#endif
#include <ncbiconf.h>
#ifndef _ARCH_BITS
#  define _ARCH_BITS NCBI_PLATFORM_BITS
#endif

#ifdef _WIN32
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x0501
#  ifndef WINDOWS
#    define WINDOWS 1
#  endif
#  include <os/win/os-native.h>
#elif defined(__linux__)
#  define LINUX 1
#  include <os/linux/os-native.h>
#elif defined(__APPLE__)
#  define MAC 1
#  include <os/mac/os-native.h>
#elif defined(__SunOS)  ||  defined(__sun__)
#  define SUN 1
#  include <os/sun/os-native.h>
#else
#  include <os/unix/unix-native.h>
#endif

#if !defined(NCBI_DLL_BUILD)  &&  !defined(_STATIC)  &&  0 /* affects stubs */
#  define _STATIC 1
#endif
