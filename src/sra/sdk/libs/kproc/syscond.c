/* $Id: syscond.c 14753 2013-03-12 20:40:34Z ucko $ */

#if defined(NCBI_WITHOUT_MT)
#  include "stcond.c"
#elif defined(_WIN32)
#  include "win/syscond.c"
#else
#  include "unix/syscond.c"
#endif
