/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#ifndef _h_os_native_
#define _h_os_native_

/* get this guy included so that off_t is 64 bit */
#ifndef _STDINT_H
#include "stdint.h"
#endif

/* specify at least NT 4.0 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

/* we should never include this directly */
#include <WINDOWS.H>
#include <limits.h>

#ifndef _h_klib_defs_
#include <klib/defs.h>
#endif

#ifndef _h_klib_text_
#include <klib/text.h>
#endif

#include <ctype.h>
#include <direct.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif



/* to make code work that depends on POSIX-bits (octal!) under Windows */
#define S_IWGRP 0020
#define S_IWOTH 0002

#define mode_t uint32_t

/*--------------------------------------------------------------------------
 * timeout_t
 *  a structure for communicating a timeout
 *  which under Windows is a relative time
 */
struct timeout_t
{
    uint32_t mS;
    uint32_t prepared;
};

/* not consistent across all windows? */
/* #define snprintf _snprintf */


#ifndef putenv
#define putenv( s ) _putenv ( s )
#endif

#define mkdir( d, m ) _mkdir( d )
#define strcasecmp _stricmp
#define strtoll _strtoi64
#define strtok_r strtok_s

#undef strdup
#define strdup( str ) \
    string_dup_measure ( ( str ), NULL )

#undef strndup
#define strndup( str, n ) \
    string_dup ( ( str ), ( n ) )

static __inline int isblank(int x)
{
    return (((x) == ' ') || ((x) == '\t'));
}

int snprintf (char * buffer, size_t bufsize, const char * format, ...);


static __inline__
void *memrchr ( const void *s, int c, size_t n )
{
    size_t i;
    const char *cp = (const char*)s;
    for ( i = n; i > 0; )
    {
        if ( ( int ) cp [ -- i ] == c )
            return (void *)(cp + i);
    }
    return NULL;
}

static __inline
char *strchrnul ( const char *s, int c_in )
{
    uint32_t i;
    for ( i=0; s[i] != 0; ++i )
    {
        if ( s[i] == c_in ) 
            break;
    }
  return ( char * )&s[ i ];
}

static __inline
char *strsep ( char **stringp, const char *delim )
{
	char *s, *tok, c, delim_char;
	const char *p_delim;

	if ( ( s = *stringp ) == NULL )
		return NULL;

	for ( tok = s; ; )
	{
		c = *s++;
		p_delim = delim;
		do {
			if ( ( delim_char = *p_delim++ ) == c )
			{
				if ( c == 0 )
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return ( tok );
			}
		} while ( delim_char != 0 );
	}
	return NULL;
}

#define gmtime_r(t, tm) gmtime_s(tm, t)
#define timegm _mkgmtime

static __inline
int strncasecmp( const char *s1, const char *s2, size_t n )
{
    return _strnicmp( s1, s2, n );
}

static __inline 
const char *strcasestr (const char *s1, const char *s2)
{
    unsigned char c2 = tolower((unsigned char) *s2);
    size_t l1 = strlen(s1), l2 = strlen(s2);
    
    if (l2 == 0) {
        return s1;
    }

    while (l1 >= l2) {
        if (tolower((unsigned char) *s1) == c2
            &&  (l2 == 1  ||  _strnicmp(s1 + 1, s2 + 1, l2 - 1) == 0)) {
            return s1;
        }
        ++s1;
        --l1;
    }

    return NULL;
}

static __inline
long int lround ( double x )
{
    double val = ( x < 0.0 ) ? ceil ( x - 0.5 ) : floor ( x + 0.5 );
    if ( val > ( double ) LONG_MAX )
        return LONG_MAX;
    if ( val < ( double ) LONG_MIN )
        return LONG_MIN;
    return ( long int ) val;
}

#ifdef __cplusplus
}
#endif

#endif /* _h_os_native_ */
