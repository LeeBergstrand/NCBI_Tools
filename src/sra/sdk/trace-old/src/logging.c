#include "logging.h"
#include "itypes.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>

static tracedb_fd_t tracedb_log_fd = 2;
static const char *tracedb_app_name_str;
static bool tracedb_loginit_called;


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
int tracedb_loginit ( const char *appname, tracedb_fd_t fd )
{
    if ( appname == NULL || appname [ 0 ] == 0 || fd < 0 )
        return EINVAL;

    tracedb_app_name_str = appname;
    tracedb_log_fd = fd;
    tracedb_loginit_called = true;

    return 0;
}

/*--------------------------------------------------------------------------
 * snoprintf
 * vsnoprintf
 *  prints into a buffer at an offset
 *
 *  by keeping the offset separate from the buffer pointer and size,
 *  buffer overruns can be detected and required sizes accumulated.
 *
 *  in the case of glib < 2.1, the required size is not known and
 *  is returned as 0.
 */
static
int tracedb_vsnoprintf ( char *buffer, size_t bsize, size_t offset,
    size_t *num_writ, const char *fmt, va_list args )
{
    int writ;
    size_t to_write;

    if ( offset >= bsize )
        to_write = 0;
    else
    {
        buffer += offset;
        to_write = bsize - offset;
    }

    writ = vsnprintf ( buffer, to_write, fmt, args );
    if ( writ < 0 )
    {
        /* glib < 2.1 */
        * num_writ = 0;
        return ENOBUFS;
    }

    * num_writ += writ;

    if ( ( size_t ) writ >= to_write )
    {
        /* glib >= 2.1 */
        return ENOBUFS;
    }

    return 0;
}

static
int tracedb_snoprintf ( char *buffer, size_t bsize, size_t offset,
    size_t *num_writ, const char *fmt, ... )
{
    int status;

    va_list args;
    va_start ( args, fmt );

    status = tracedb_vsnoprintf ( buffer, bsize, offset, num_writ, fmt, args );

    va_end ( args );

    return status;
}


/*--------------------------------------------------------------------------
 * timestamp
 *  returns the current time in GMT
 */
static
int tracedb_timestamp ( char *buffer, size_t bsize, size_t offset,
    size_t *num_writ )
{
    static time_t last_time = 0;
    static struct tm cal;
    
    /* get current time */
    time_t t = time ( 0 );
    
    /* initialize time on first run */
    if ( ! last_time )
    {
        last_time = t;
        gmtime_r ( & last_time, & cal );
    }
    
    /* or update if time has passed */
    else if ( t != last_time )
    {
        /* update every 5 minutes or so */
        time_t dt = t - last_time;
        last_time = t;
        if ( dt >= 300 )
            gmtime_r ( & last_time, & cal );
	
        /* otherwise, just update the struct manually */
        else
        {
            /* advance seconds */
            dt += cal . tm_sec;
            cal . tm_sec = ( int ) ( dt % 60 );
	    
            /* detect a rolled-over minute */
            if ( ( dt /= 60 ) != 0 )
            {
                /* advance minutes */
                dt += cal . tm_min;
                cal . tm_min = ( int ) ( dt % 60 );
		
                /* detect a rolled-over hour */
                if ( ( dt /= 60 ) != 0 )
                {
                    /* roll-over of an hour - refetch */
                    gmtime_r ( & last_time, & cal );
                }
            }
        }
    }
    
    /* make the timestamp */
    return tracedb_snoprintf ( buffer, bsize, offset, num_writ,
                       "%04d-%02d-%02d %02d:%02d:%02d ",
                       cal . tm_year + 1900,
                       cal . tm_mon + 1,
                       cal . tm_mday,
                       cal . tm_hour,
                       cal . tm_min,
                       cal . tm_sec );
}


/*--------------------------------------------------------------------------
 * app_name
 *  returns a previously recorded app name
 *  or else a numeric id of the current process
 */
static
int tracedb_app_name ( char *buffer, size_t bsize, size_t offset,
    size_t *num_writ )
{
    /* detect case where loginit has not been called
       or the app name has been set to NULL */
    if ( ! tracedb_app_name_str )
    {
        static char pid [ 32 ];
        snprintf ( pid, sizeof pid, "pid #%d", getpid () );
        tracedb_app_name_str = pid;
	
        /* detect case where invoked prior to
           static initialization code being run */
        if ( ! tracedb_log_fd && ! tracedb_loginit_called )
            tracedb_log_fd = 2;
    }
    
    return tracedb_snoprintf ( buffer, bsize, offset, num_writ,
        "%s: ", tracedb_app_name_str );
}


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
static
int tracedb_vlogstr_int ( char *buffer, size_t bsize, size_t *num_writ,
    const char *fmt, va_list args )
{
    /* start out with an empty buffer */
    size_t total = 0;

    /* tack on a timestamp */
    int status = tracedb_timestamp ( buffer, bsize, 0, & total );
    if ( status == 0 || ( status == ENOBUFS && total > 0 ) )
    {
        /* tack on application name */
        status = tracedb_app_name ( buffer, bsize, total, & total );
        if ( status == 0 || ( status == ENOBUFS && total > 0 ) )
        {
            /* print message */
            status = tracedb_vsnoprintf ( buffer, bsize, total,
                & total, fmt, args );

            /* convert bytes printed into buffer size on error */
            if ( status == ENOBUFS && total != 0 )
                ++ total;
        }
    }

    * num_writ = total;
    return status;
}

int tracedb_vlogstr ( char *buffer, size_t bsize, size_t *num_writ,
    const char *fmt, va_list args )
{
    int status;
    size_t total;

    if ( num_writ == NULL )
        num_writ = & total;

    status = tracedb_vlogstr_int ( buffer, bsize, num_writ, fmt, args );
    if ( status == ENOBUFS )
    {
        /* detect glib < 2.1 */
        if ( * num_writ == 0 )
        {
            /* if the buffer was already large, just abort */
            if ( bsize >= 16 * 1024 )
                return -1;

            /* allocate a new buffer */
            buffer = malloc ( 16 * 1024 );
            if ( buffer == NULL )
                return errno;

            /* generate string and discard just to get a size */
            status = tracedb_vlogstr_int ( buffer, 16 * 1024, num_writ,
                fmt, args );
            free ( buffer );

            /* map buffer size to unknown */
            if ( status == ENOBUFS )
                status = -1;
        }
    }

    return status;
}

int tracedb_logstr ( char *buffer, size_t bsize, size_t *num_writ,
    const char *fmt, ... )
{
    int status;

    va_list args;
    va_start ( args, fmt );

    status = tracedb_vlogstr ( buffer, bsize, num_writ, fmt, args );

    va_end ( args );

    return status;
}


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
int tracedb_vlogmsg ( const char *fmt, va_list args )
{
    char buffer [ 512 ], *str = buffer;
    size_t num_writ, bsize = sizeof buffer;

    /* try writing into little buffer on stack */
    int status = tracedb_vlogstr_int ( buffer, sizeof buffer,
        & num_writ, fmt, args );
    if ( status == ENOBUFS )
    {
        /* detect glib < 2.1 and convert >= 2.1 into
           enough space for an added newline */
        if ( num_writ ++ == 0 )
            num_writ = 16 * 1024;

        /* allocate a buffer for very large writes */
        str = malloc ( num_writ );
        if ( str == NULL )
            return errno;

        /* re-write */
        status = tracedb_vlogstr_int ( str, bsize = num_writ,
            & num_writ, fmt, args );
    }

    /* if the buffer was successfully written */
    if ( status == 0 )
    {
        /* check for need to nl terminate */
        bool nlterm = false;
        if ( str [ num_writ - 1 ] != '\n' )
        {
            /* see if there's any more room in buffer */
            if ( num_writ == bsize )
                nlterm = true;
            else
                str [ num_writ ++ ] = '\n';
        }

        /* write entire guy */
        if ( write ( tracedb_log_fd, str, num_writ ) < 0 )
            status = errno;

        /* although not atomic, tack on a newline if needed */
        else if ( nlterm )
        {
            if ( write ( tracedb_log_fd, "\n", 1 ) < 0 )
                status = errno;
        }
    }

    if ( str != buffer )
        free ( str );

    return status;
}

int tracedb_logmsg ( const char *fmt, ... )
{
    int status;

    va_list args;
    va_start ( args, fmt );

    status = tracedb_vlogmsg ( fmt, args );

    va_end ( args );

    return status;
}


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
int tracedb_vlogerr ( int err, const char *fmt, va_list args )
{
    /* buffer space */
    char msg [ 256 ];
#if STRERROR_R_NOT_BROKEN
    char buff [ 128 ];
#endif

    /* write the formatted message */
    size_t num_writ = 0;
    int status = tracedb_vsnoprintf ( msg, sizeof msg, 0,
        & num_writ, fmt, args );

    /* truncate to message buffer */
    if ( status == ENOBUFS )
        msg [ num_writ = sizeof msg - 1 ] = 0;
    else if ( status != 0 )
        return status;

    /* if the guy sent us a newline at the end,
       just shorten the message because we are
       going to extend the line */
    if ( msg [ -- num_writ ] == '\n' )
        msg [ num_writ ] = 0;

    /* strerror_r is supposed to return an int, but under
       GNU glibc 2.0 and beyond, it returns a const char* */
#if STRERROR_R_NOT_BROKEN
    status = strerror_r ( err, buff, sizeof buff );
    if ( status < 0 )
        return errno;
    return tracedb_logmsg ( "ERROR: %s - %s\n", msg, buff );
#else
    return tracedb_logmsg ( "ERROR: %s - %s\n", msg, strerror ( err ) );
#endif
}

int tracedb_logerr ( int err, const char *fmt, ... )
{
    int status;

    va_list args;
    va_start ( args, fmt );

    status = tracedb_vlogerr ( err, fmt, args );

    va_end ( args );

    return status;
}
