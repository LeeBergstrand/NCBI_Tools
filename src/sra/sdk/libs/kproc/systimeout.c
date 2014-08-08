/* $Id: systimeout.c 14717 2013-03-08 15:25:05Z ucko $ */
 
#if defined(NCBI_WITHOUT_MT)
#  include "sttimeout.c"
#elif defined(_WIN32)
#  include "win/systimeout.c"
#else
#  include "unix/systimeout.c"
#endif
