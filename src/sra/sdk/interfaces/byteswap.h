/* $Id: byteswap.h 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef __GNUC__
#  ifdef __i386__
#    include <cc/gcc/i386/byteswap.h>
#  elif defined(__x86_64__)
#    include <cc/gcc/x86_64/byteswap.h>
#  endif
#endif

#ifndef _h_byteswap_
#  ifdef __APPLE__
#    include <os/mac/byteswap.h>
#  elif defined(__SunOS)  ||  defined(__sun__)
#    include <os/sun/byteswap.h>
#  elif defined(_WIN32)
#    include <os/win/byteswap.h>
#  elif defined(__linux__)
#    include_next <byteswap.h>
#  else
#    error "Unsupported OS"
#  endif
#endif
