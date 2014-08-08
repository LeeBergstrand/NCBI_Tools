/* $Id: stdint.h 14717 2013-03-08 15:25:05Z ucko $ */

#if defined(_DEBUG)  &&  !defined(_DEBUGGING)
#  define _DEBUGGING _DEBUG
#endif

#ifdef _MSC_VER
#  include <cc/vc++/stdint.h>
#elif defined(__GNUC__)
#  include_next <stdint.h>
#else
#  include <../include/stdint.h>
#endif
