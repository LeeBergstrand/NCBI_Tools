/* $Id: syscond-priv.h 14717 2013-03-08 15:25:05Z ucko $ */

#if defined(_WIN32)
#  include "win/syscond-priv.h"
#else
#  include "unix/syscond-priv.h"
#endif
