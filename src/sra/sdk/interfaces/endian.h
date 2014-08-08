/* $Id: endian.h 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef __APPLE__
#  include <os/mac/endian.h>
#elif defined(__SunOS)  ||  defined(__sun__)
#  include <os/sun/endian.h>
#elif defined(_WIN32)
#  include <os/win/endian.h>
#elif defined(__linux__)
#  include_next <endian.h>
#else
#  error "Unsupported OS"
#endif
