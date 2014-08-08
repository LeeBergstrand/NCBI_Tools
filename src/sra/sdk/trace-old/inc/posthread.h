#ifndef _h_posthread_
#define _h_posthread_

/* hack for getting rwlocks defined */
#include <stdint.h>
#include <stddef.h>

#ifndef __USE_UNIX98
#define __USE_UNIX98 1
#endif

#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K 1
#endif

#include <pthread.h>

#define TRACEDB_HAVE_TMLOCK 1

#if defined __USE_UNIX98 && defined __USE_XOPEN2K
#define TRACEDB_HAVE_RWLOCK 1
#else
#define TRACEDB_HAVE_RWLOCK 0
#endif

#endif /* _h_posthread_ */
