/* $Id */

#define JUDY1 1
#include "judy-arch.h"
#ifdef JU_32BIT
#  include "judy/Judy1Tables-32.c"
#else
#  include "judy/Judy1Tables-64.c"
#endif
