/* $Id: compiler.h 14717 2013-03-08 15:25:05Z ucko $ */

#if defined(__ICC)  ||  defined(__INTEL_COMPILER)
#  include <cc/icc/compiler.h>
#elif defined(__GNUC__)
#  include <cc/gcc/compiler.h>
#elif defined(_MSC_VER)
#  include <cc/vc++/compiler.h>
#else
#  include <cc/cc/compiler.h>
#endif
