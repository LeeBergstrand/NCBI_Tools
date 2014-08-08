/* $Id: arch-impl.h 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef __GNUC__
#  ifdef __i386__
#    include <cc/gcc/i386/arch-impl.h>
#  elif defined(__x86_64__)
#    include <cc/gcc/x86_64/arch-impl.h>
#  elif defined(__powerpc__)  ||  defined(__ppc__)
#    include <cc/gcc/ppc32/arch-impl.h>
#  endif
#elif defined(_MSC_VER)
#  if defined(_M_X64)  &&  _MSC_VER > 1600
/* Needs __int128, which remains unavailable in Visual Studio (20)10. */
#    include <cc/vc++/x86_64/arch-impl.h>
#  elif defined(_M_IX86)  ||  defined(_M_X64)
#    include <cc/vc++/i386/arch-impl.h>
#  endif
#endif

#ifndef _h_arch_impl_
#  if defined(__SunOS)  ||  defined(__sun__)
#    include <os/sun/arch-impl.h>
#  else
#    error "Unsupported platform"
#  endif
#endif
