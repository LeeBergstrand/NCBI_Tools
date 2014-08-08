/* $Id: systhread.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef NCBI_WITHOUT_MT
#  include "stthread.c"
#elif defined(_WIN32)
#  include "win/systhread.c"
#else
#  include "unix/systhread.c"
#endif
