/* $Id: xsem.c 14717 2013-03-08 15:25:05Z ucko $ */

#if defined(NCBI_WITHOUT_MT)  ||  (defined(_WIN32) && defined(_USRDLL))
/* sem.c needs syscond.c, which hasn't been ported. */
#  include "stsem.c"
#else
#  include "sem.c"
#endif
