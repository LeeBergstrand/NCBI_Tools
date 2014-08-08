/* $Id: syspath.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32
#  include "win/syspath.c"
#else
#  include "unix/syspath.c"
#endif
