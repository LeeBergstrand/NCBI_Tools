#ifndef _h_itypes_
#define _h_itypes_

/* pick up standard integer types */
#include <stdint.h>

/* pick up mode_t and etc */
#include <sys/types.h>

/* pick up size_t and friends */
#include <stddef.h>

#if !defined(bool) && !defined(bool_defined) && !defined(HAVE_BOOL) && !defined(__cplusplus)
typedef uint8_t bool;
enum { false, true };
#define HAVE_BOOL
#endif

#endif /* _h_itypes_ */
