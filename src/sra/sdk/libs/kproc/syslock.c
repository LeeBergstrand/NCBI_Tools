/* $Id: syslock.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef NCBI_WITHOUT_MT
#  include "stlock.c"
#elif defined(__linux__)
#  include "linux/syslock.c"
#elif defined(__FreeBSD__)  ||  defined(__NetBSD__)  ||  defined(__OpenBSD__) \
  ||  defined(__APPLE__)
#  include "bsd/syslock.c"
#elif defined(__SunOS)  ||  defined(__sun__)
#  include "sun/syslock.c"
#elif defined(_WIN32)
#  include "win/syslock.c"
#else
#  ifdef __GNUC__
#    warning "Unsupported OS; falling back to ST lock"
#  endif
#  include "stlock.c"
#endif
