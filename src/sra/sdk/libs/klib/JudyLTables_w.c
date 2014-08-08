/* $Id */

#define JUDYL 1
#include "judy-arch.h"
#ifdef JU_32BIT
#  include "judy/JudyLTables-32.c"
#else
#  include "judy/JudyLTables-64.c"
#endif
