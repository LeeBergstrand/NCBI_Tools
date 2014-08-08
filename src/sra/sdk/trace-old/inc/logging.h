#ifndef _h_logging_
#define _h_logging_

#ifndef _h_fdio_
#include "fdio.h"
#endif

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * loginit
 *  record basic application identification for logging
 *
 *  if logging is invoked prior to or in absence of
 *  logint, the default behavior is to use the running
 *  process' pid as appname and fd 2.
 *
 *  "appname" [ IN ] - NULL terminated const string with
 *  guaranteed process lifetime, as the pointer is recorded
 *  but the string not copied.
 *
 *  "fd" [ IN ]  - file descriptor where logging output will
 *  be written, normally with an id of 2 ( stderr )
 *
 *  return values:
 *    EINVAL
 */
int tracedb_loginit ( const char *appname, tracedb_fd_t fd );


/*--------------------------------------------------------------------------
 * logmsg
 *  makes an entry to the log file
 *
 *  every invocation prints a timestamp and appname
 *  ( or pid if appname is not set ) followed by a
 *  printf-style rendering of the supplied arguments.
 *
 *  every invocation ensures that the output terminates
 *  with a newline, meaning that a newline will be supplied
 *  if missing.
 *
 *  embedded newlines are allowed and do NOT cause
 *  new timestamps to be generated.
 *
 *  "fmt" [ IN ] - NULL terminated UTF-8 format string
 *  ( see man format for more information )
 *
 *  "args" [ IN ] - format arguments
 *
 *  return values:
 */
int tracedb_logmsg ( const char *fmt, ... );
int tracedb_vlogmsg ( const char *fmt, va_list args );


/*--------------------------------------------------------------------------
 * logstr
 *  makes an entry to the supplied character buffer
 *
 *  acts just like logmsg except to a character buffer
 *
 *  "buffer" [ OUT ]  and "bsize" [ IN ] - return parameter for
 *  gathering log string. NULL terminated when successful.
 *
 *  "num_writ" [ OUT, NULL OKAY ] - contains the number of
 *  valid bytes MINUS terminating NULL in buffer on success,
 *  or the required bsize on ENOBUFS ( including NULL ).
 *  invalid otherwise.
 *
 *  "fmt" [ IN ] - NULL terminated UTF-8 format string
 *  ( see man format for more information )
 *
 *  "args" [ IN ] - format arguments
 *
 *  return values:
 *    ENOBUFS - buffer too small but num_writ contains bytes 
 */
int tracedb_logstr ( char *buffer, size_t bsize, size_t *num_writ, const char *fmt, ... );
int tracedb_vlogstr ( char *buffer, size_t bsize, size_t *num_writ, const char *fmt, va_list args );

/*--------------------------------------------------------------------------
 * logerr
 *  like perror, but takes its error code as a parameter
 *  and prints a timestamp
 *
 *  invokes log with message and output of strerror.
 *
 *  "err" [ IN ] - error code
 *
 *  "fmt" [ IN ] - NULL terminated UTF-8 format string
 *
 *  "args" [ IN ] - format arguments
 *
 *  return values:
 */
int tracedb_logerr ( int err, const char *fmt, ... );
int tracedb_vlogerr ( int err, const char *fmt, va_list args );


#ifdef __cplusplus
}
#endif

#endif /* _h_logging_ */
