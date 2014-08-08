/* $Id: sysmain.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32
#  include "win/sysmain.c"
#elif defined(__SunOS)  ||  defined(__sun__)
#  include "sun/sysmain.c"
#else
#  include "unix/sysmain.c"
#endif
