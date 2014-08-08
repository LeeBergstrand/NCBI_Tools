/* $Id: systime.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32
#  include "win/systime.c"
#elif defined(__SunOS)  ||  defined(__sun__)
#  include "sun/systime.c"
#else
#  include "unix/systime.c"
#endif

