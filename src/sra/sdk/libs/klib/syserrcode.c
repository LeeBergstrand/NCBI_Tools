/* $Id: syserrcode.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32
#  include "win/syserrcode.c"
#elif defined(__FreeBSD__)  ||  defined(__NetBSD__)  ||  defined(__OpenBSD__) \
  ||  defined(__APPLE__)
#  include "bsd/syserrcode.c"
#elif defined(__linux__)
#  include "linux/syserrcode.c"
#else
#  include "unix/syserrcode.c"
#endif

