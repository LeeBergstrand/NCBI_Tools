/* $Id: syslock-priv.h 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef __linux__
#  include "linux/syslock-priv.h"
#elif defined(__FreeBSD__)  ||  defined(__NetBSD__)  ||  defined(__OpenBSD__) \
  ||  defined(__APPLE__)
#  include "bsd/syslock-priv.h"
#elif defined(__SunOS)  ||  defined(__sun__)
#  include "sun/syslock-priv.h"
#elif defined(_WIN32)
#  include "win/syslock-priv.h"
#else
#  ifdef __GNUC__
#    warning "Unsupported OS; falling back to ST lock"
#  endif
#endif
