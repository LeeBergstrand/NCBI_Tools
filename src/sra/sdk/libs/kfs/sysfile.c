/* $Id: sysfile.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32
#  include "win/sysfile.c"
#else
#  include "unix/sysfile.c"
#endif
