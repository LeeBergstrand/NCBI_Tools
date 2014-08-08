/* $Id: sysalloc.h 14717 2013-03-08 15:25:05Z ucko $ */

#if defined(_DEBUG)  &&  !defined(_DEBUGGING)
#  define _DEBUGGING _DEBUG
#endif

#ifdef _WIN32
#  include <os/win/sysalloc.h>
#else
#  include <os/unix/sysalloc.h>
#endif
