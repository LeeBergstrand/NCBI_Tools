/* $Id: sysbarrier.c 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef NCBI_WITHOUT_MT
#  include "stbarrier.c"
#elif defined(__linux__)
#  include "linux/sysbarrier.c"
#elif defined(__SunOS)  ||  defined(__sun__)
#  include "sun/sysbarrier.c"
#else
#  ifdef __GNUC__
#    warning "Unsupported OS; falling back to ST barrier"
#  endif
#  include "stbarrier.c"
#endif
