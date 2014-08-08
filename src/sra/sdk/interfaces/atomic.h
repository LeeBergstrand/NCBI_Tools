/* $Id: atomic.h 14717 2013-03-08 15:25:05Z ucko $ */

#include <ncbiconf.h> /* conditionalize? */
#ifndef _ARCH_BITS
#  define _ARCH_BITS NCBI_PLATFORM_BITS
#endif

#ifdef __GNUC__
#  ifdef __i386__
#    include <cc/gcc/i386/atomic.h>
#  elif defined(__x86_64__)
#    include <cc/gcc/x86_64/atomic.h>
#  elif defined(__ppc__) || defined(__powerpc__)
#    include <cc/gcc/ppc32/atomic.h>
#  endif
#endif

#ifndef _h_atomic_
#  if defined(__SunOS)  ||  defined(__sun__)
#    include <os/sun/atomic.h>
#  elif defined(_WIN32)
#    include <os/win/atomic.h>
#  else
#    error "Unsupported platform"
#  endif
#endif
