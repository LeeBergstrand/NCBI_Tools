/* $Id: atomic32.h 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef __GNUC__
#  ifdef __i386__
#    include <cc/gcc/i386/atomic32.h>
#  elif defined(__x86_64__)
#    include <cc/gcc/x86_64/atomic32.h>
#  elif defined(__ppc__) || defined(__powerpc__)
#    include <cc/gcc/ppc32/atomic32.h>
#  endif
#endif

#ifndef _h_atomic32_
#  if defined(__SunOS)  ||  defined(__sun__)
#    include <os/sun/atomic32.h>
#  elif defined(_WIN32)
#    include <os/win/atomic32.h>
#  else
#    error "Unsupported platform"
#  endif
#endif
