/* $Id: syspass.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32
#  include "win/syspass.c"
#elif defined(__APPLE__)
#  include "mac/syspass.c"
#else
#  include "unix/syspass.c"
#endif
