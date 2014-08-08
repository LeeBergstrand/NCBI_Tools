#ifndef _h_fmtdef_
#define _h_fmtdef_

/* int64_t is "long long int" in 32 bit */
#define TRACEDB_LD64 "lld"
#define TRACEDB_LU64 "llu"

/* size_t is traditionally "unsigned int"
   although it should have been "long unsigned int" */
#define TRACEDB_LUSZ "u"

#endif /* _h_fmtdef_ */
