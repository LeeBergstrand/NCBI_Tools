/* $Id: va_copy.h 14717 2013-03-08 15:25:05Z ucko $ */

#if defined(__GNUC__)
#  include <cc/gcc/va_copy.h>
#elif defined(_MSC_VER)
#  include <cc/vc++/va_copy.h>
#elif defined(__SunOS)  ||  defined(__sun__)
#  include <os/sun/va_copy.h>
#else
#  error "Unsupported platform"
#endif
