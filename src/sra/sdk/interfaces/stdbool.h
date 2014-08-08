/* $Id: stdbool.h 14717 2013-03-08 15:25:05Z ucko $ */

#ifdef _MSC_VER
#  include <cc/vc++/stdbool.h>
#elif defined(__GNUC__)
#  include_next <stdbool.h>
#elif !defined(__cplusplus)
#  define _STDC_C99 1
#  include <../include/stdbool.h>
#endif
