/* $Id: syslockfile.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _WIN32
#  include "win/syslockfile.c"
#else
#  include "unix/syslockfile.c"
#endif
