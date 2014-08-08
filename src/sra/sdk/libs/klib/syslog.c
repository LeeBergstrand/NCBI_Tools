/* $Id: syslog.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32
#  include "win/syslog.c"
#elif defined(__SunOS)  ||  defined(__sun__)
#  include "sun/syslog.c"
#else
#  include "unix/syslog.c"
#endif

