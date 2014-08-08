/* $Id: bitstr.h 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef __GNUC__
#  ifdef __i386__
#    include <cc/gcc/i386/bitstr.h>
#  elif defined(__x86_64__)
#    include <cc/gcc/x86_64/bitstr.h>
#  elif defined(__powerpc__)  ||  defined(__ppc__)
#    include <cc/gcc/ppc32/bitstr.h>
#  endif
#elif defined(_MSC_VER)
#  ifdef _M_IX86
#    include <cc/vc++/i386/bitstr.h>
#  elif defined(_M_X64)
#    include <cc/vc++/x86_64/bitstr.h>
#  endif
#endif

#ifndef _h_bitstr_
#  if defined(__SunOS)  ||  defined(__sun__)
#    include <os/sun/bitstr.h>
#  else
#    error "Unsupported platform"
#  endif
#endif
