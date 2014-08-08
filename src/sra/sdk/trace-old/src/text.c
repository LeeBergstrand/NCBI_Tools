#include "text.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

/*--------------------------------------------------------------------------
 * tracedb_String
 *  pseudo-intrinsic string
 */

/* tracedb_StringCopy
 *  allocates a copy of a string
 */
int tracedb_StringCopy ( const tracedb_String **cpy, const tracedb_String *str )
{
    tracedb_String *s;
    char *addr;
    size_t size;

    if ( cpy == NULL )
        return EINVAL;

    if ( str == NULL )
    {
        * cpy = NULL;
        return EINVAL;
    }

    size = str -> size;
    s = ( tracedb_String* ) malloc ( sizeof * s + size + 1 );
    if ( s == NULL )
    {
        * cpy = NULL;
        return ENOMEM;
    }
    * cpy = s;

    addr = ( char* ) ( s + 1 );
    s -> addr = addr;
    s -> size = size;
    s -> len = str -> len;
    memcpy ( addr, str -> addr, size );
    addr [ size ] = 0;

    return 0;
}

/* tracedb_StringConcat
 *  concatenate one string onto another
 */
int tracedb_StringConcat ( const tracedb_String **cat, const tracedb_String *a, const tracedb_String *b )
{
    tracedb_String *s;
    char *addr;
    size_t size;

    if ( cat == NULL )
        return EINVAL;

    if ( a == NULL || b == NULL )
    {
        * cat = NULL;
        return EINVAL;
    }

    size = a -> size + b -> size;
    s = ( tracedb_String* ) malloc ( sizeof * s + size + 1 );
    if ( s == NULL )
    {
        * cat = NULL;
        return ENOMEM;
    }
    * cat = s;

    addr = ( char* ) ( s + 1 );
    s -> addr = addr;
    s -> size = size;
    s -> len = a -> len + b -> len;
    memcpy ( addr, a -> addr, a -> size );
    memcpy ( addr + a -> size, b -> addr, b -> size );
    addr [ size ] = 0;

    return 0;
}

/* tracedb_StringSubstr
 *  creates a substring of an existing one
 *  note that the substring is always a non-allocated copy
 *  and is dependent upon the lifetime of its source
 *
 *  returns "sub" if "idx" was valid
 *  or NULL otherwise
 *
 *  "len" may be 0 to indicate infinite length
 *  or may extend beyond end of source string.
 */
tracedb_String *tracedb_StringSubstr ( const tracedb_String *str, tracedb_String *sub,
    unsigned int idx, unsigned int len )
{
    if ( str != NULL && sub != NULL && idx < str -> len )
    {
        const char *addr = tracedb_string_idx ( str -> addr, str -> size, idx );
        if ( addr != NULL )
        {
            sub -> size = str -> size - ( size_t ) ( addr - str -> addr );
            sub -> len = str -> len - idx;
            sub -> addr = addr;
            if ( len > 0 && len < sub -> len )
            {
                const char *end = tracedb_string_idx ( sub -> addr, sub -> size, len );
                if ( end != NULL )
                {
                    sub -> size = ( size_t ) ( end - sub -> addr );
                    sub -> len = len;
                }
            }
            return sub;
        }
    }
    return NULL;
}

/* tracedb_StringHead
 *  access the first character
 *
 *  this is an efficient enough function to be included.
 *  the generic functions of accessing characters by index
 *  are apt to be extremely inefficient with UTF-8, and
 *  as such are not included.
 *
 *  returns EINVAL if the character is bad,
 *  or ENODATA if the string is empty
 */
int tracedb_StringHead ( const tracedb_String *str, unsigned int *ch )
{
    int ch_len;

    if ( str == NULL || ch == NULL )
        return EINVAL;

    if ( str -> len == 0 )
    {
        * ch = 0;
        return ENODATA;
    }

    ch_len = tracedb_utf8_utf32 ( ch, str -> addr, str -> addr + str -> size );
    if ( ch_len > 0 )
        return 0;

    * ch = 0;
    return EINVAL;
}

/* tracedb_StringPopHead
 *  remove and return the first character
 *
 *  returns EINVAL if the character is bad,
 *  or ENODATA if the string is empty
 */
int tracedb_StringPopHead ( tracedb_String *str, unsigned int *ch )
{
    int ch_len;

    if ( str == NULL || ch == NULL )
        return EINVAL;

    if ( str -> len == 0 )
    {
        * ch = 0;
        return ENODATA;
    }

    ch_len = tracedb_utf8_utf32 ( ch, str -> addr, str -> addr + str -> size );
    if ( ch_len <= 0 )
    {
        * ch = 0;
        return EINVAL;
    }

    str -> addr += ch_len;
    -- str -> len;
    str -> size -= ch_len;

    return 0;
}

/* tracedb_StringCompare
 *  compare strings for relative ordering
 */
int tracedb_StringCompare ( const tracedb_String *a, const tracedb_String *b )
{
    int diff;
    unsigned int len;

    if ( a == b )
        return 0;
    if ( a == NULL )
        return -1;
    if ( b == NULL )
        return 1;

    len = a -> len;
    if ( b -> len < len )
        len = b -> len;

    diff = tracedb_string_cmp ( a -> addr, a -> size, b -> addr, b -> size, len );
    if ( diff == 0 )
        diff = ( int ) a -> len - ( int ) b -> len;
    return diff;
}

/* tracedb_StringCaseEqual
 *  compare strings for case-insensitive equality
 */
bool tracedb_StringCaseEqual ( const tracedb_String *a, const tracedb_String *b )
{
    unsigned int len;

    if ( a == b )
        return true;
    if ( a == NULL || b == NULL )
        return false;

    len = a -> len;
    if ( b -> len != len )
        return false;

    return ( bool ) ( tracedb_strcase_cmp ( a -> addr, a -> size, b -> addr, b -> size, len ) == 0 );
}

/* tracedb_StringCaseCompare
 *  compare strings for relative case-insensitive ordering
 */
int tracedb_StringCaseCompare ( const tracedb_String *a, const tracedb_String *b )
{
    int diff;
    unsigned int len;

    if ( a == b )
        return 0;
    if ( a == NULL )
        return -1;
    if ( b == NULL )
        return 1;

    len = a -> len;
    if ( b -> len < len )
        len = b -> len;

    diff = tracedb_strcase_cmp ( a -> addr, a -> size, b -> addr, b -> size, len );
    if ( diff == 0 )
        diff = ( int ) a -> len - ( int ) b -> len;
    return diff;
}

/* tracedb_StringMatch
 *  creates a substring of "a" in "match"
 *  for all of the sequential matching characters between "a" and "b"
 *  starting from character [ 0 ].
 *
 *  returns the number of characters that match.
 */
unsigned int tracedb_StringMatch ( tracedb_String *match,
    const tracedb_String *a, const tracedb_String *b )
{
    if ( a == NULL || b == NULL )
    {
        TRACEDB_CONST_STRING ( match, "" );
        return 0;
    }

    if ( a == b )
    {
        * match = * a;
        return a -> len;
    }

    if ( match == NULL )
    {
        return tracedb_string_match ( a -> addr, a -> size,
            b -> addr, b -> size, b -> size, NULL );
    }

    match -> addr = a -> addr;
    return match -> len = tracedb_string_match ( a -> addr, a -> size,
        b -> addr, b -> size, b -> size, & match -> size );
}

/* tracedb_StringMatchExtend
 *  extends a substring of "a" in "match"
 *  for all of the sequential matching characters between "a" and "b"
 *  starting from character [ match -> len ].
 *
 *  returns the number of matching characters that were extended.
 */
unsigned int tracedb_StringMatchExtend ( tracedb_String *match, const tracedb_String *a, const tracedb_String *b )
{
    size_t msize;
    unsigned int len;

    assert ( match != NULL );
    if ( match -> len == 0 )
        return tracedb_StringMatch ( match, a, b );

    assert ( a != NULL );
    assert ( match -> addr == a -> addr );
    assert ( match -> len <= a -> len );

    if ( b == NULL || match -> len == a -> len || match -> len >= b -> len )
        return 0;

    msize = match -> size;
    len = tracedb_string_match ( a -> addr + msize, a -> size - msize,
        b -> addr + msize, b -> size - msize, b -> size, & msize );

    match -> len += len;
    match -> size += msize;
    return len;
}

/* tracedb_StringCopyUTF...
 *  creates a tracedb_String from UTF16 or UTF32 UNICODE input
 *  wchar_t is one or the other, depending upon OS and compiler.
 */
int tracedb_StringCopyUTF16 ( const tracedb_String **cpy, const uint16_t *text, size_t bytes )
{
    size_t size;
    unsigned int len = tracedb_utf16_cvt_string_len ( text, bytes, & size );
    tracedb_String *str = ( tracedb_String* ) malloc ( sizeof * str + 1 + size );
    if ( ( * cpy = str ) == NULL )
        return ENOMEM;
    tracedb_StringInit ( str, ( char* ) ( str + 1 ), size, len );
    str -> size = tracedb_utf16_cvt_string_copy ( ( char* ) str -> addr, size, text, bytes );
    return 0;
}

int tracedb_StringCopyUTF32 ( const tracedb_String **cpy, const uint32_t *text, size_t bytes )
{
    size_t size;
    unsigned int len = tracedb_utf32_cvt_string_len ( text, bytes, & size );
    tracedb_String *str = ( tracedb_String* ) malloc ( sizeof * str + 1 + size );
    if ( ( * cpy = str ) == NULL )
        return ENOMEM;
    tracedb_StringInit ( str, ( char* ) ( str + 1 ), size, len );
    str -> size = tracedb_utf32_cvt_string_copy ( ( char* ) str -> addr, size, text, bytes );
    return 0;
}

int tracedb_StringCopyWChar_t ( const tracedb_String **cpy, const wchar_t *text, size_t bytes )
{
    size_t size;
    unsigned int len = tracedb_wchar_cvt_string_len ( text, bytes, & size );
    tracedb_String *str = ( tracedb_String* ) malloc ( sizeof * str + 1 + size );
    if ( ( * cpy = str ) == NULL )
        return ENOMEM;
    tracedb_StringInit ( str, ( char* ) ( str + 1 ), size, len );
    str -> size = tracedb_wchar_cvt_string_copy ( ( char* ) str -> addr, size, text, bytes );
    return 0;
}

/*--------------------------------------------------------------------------
 * raw text strings
 */

/* tracedb_string_size
 *  length of string in bytes
 */
size_t tracedb_string_size ( const char *str )
{
    assert ( str != NULL );
    if ( str == NULL )
        return 0;
    return strlen ( str );
}

/* tracedb_string_printf
 * tracedb_string_vprintf
 *  much like snprintf except that it handles overflows
 *
 *  returns the number of bytes written in num_writ on success,
 *  or the required buffer size ( including null byte ) on ENOBUFS
 *
 *  return value is an error code
 */
int tracedb_string_vprintf ( char *dst, size_t bsize, size_t *num_writ, const char *fmt, va_list args )
{
    int writ;

    assert ( dst != NULL );
    writ = vsnprintf ( dst, bsize, fmt, args );
    if ( writ >= 0 && ( size_t ) writ < bsize )
    {
        if ( num_writ != NULL )
            * num_writ = writ;
        return 0;
    }

    while ( writ < 0 )
    {
        /* glib < 2.1 */
        dst = malloc ( bsize += bsize );
        if ( dst == NULL )
            return errno;
        writ = vsnprintf ( dst, bsize, fmt, args );
        free ( dst );
    }

    if ( num_writ != NULL )
        * num_writ = writ + 1;
    return ENOBUFS;
}

int tracedb_string_printf ( char *dst, size_t bsize, size_t *num_writ, const char *fmt, ... )
{
    int status;

    va_list args;
    va_start ( args, fmt );

    status = tracedb_string_vprintf ( dst, bsize, num_writ, fmt, args );

    va_end ( args );

    return status;
}

/* tracedb_string_hash
 *  hashes a string
 */
unsigned int tracedb_string_hash ( const char *str, size_t size )
{
    size_t i;
    unsigned int hash;

    assert ( str != NULL );
    if ( str == NULL )
        return 0;

    for ( hash = 0, i = 0; i < size; ++ i )
    {
        unsigned int ch = ( ( const unsigned char* )  str ) [ i ];
        hash = ( ( hash << 1 ) - ( hash >> 16 ) ) ^ ch;
    }
    return hash ^ ( hash >> 16 );
}

/* tracedb_utf8_utf32
 *  converts UTF8 text to a single UTF32 character
 *  returns the number of UTF8 bytes consumed, such that:
 *    return > 0 means success
 *    return == 0 means insufficient input
 *    return < 0 means bad input or bad argument
 */
int tracedb_utf8_utf32 ( unsigned int *dst, const char *begin, const char *end )
{
    int c;
    unsigned int ch;
    const char *src, *stop;

    if ( dst == NULL || begin == NULL || end == NULL )
        return -1;

    if ( begin == end )
        return 0;

    /* non-negative bytes are ASCII-7 */
    c = begin [ 0 ];
    if ( begin [ 0 ] >= 0 )
    {
        dst [ 0 ] = c;
        return 1;
    }
	
    /* the leftmost 24 bits are set
       the rightmost 8 can look like:
       110xxxxx == 2 byte character
       1110xxxx == 3 byte character
       11110xxx == 4 byte character
       111110xx == 5 byte character
       1111110x == 6 byte character
    */
	
    src = begin;
	
    /* invert bits to look at range */
    ch = c;
    c = ~ c;
	
    /* illegal range */
    if ( c >= 0x40 )
        return -1;
	
    /* 2 byte */
    else if ( c >= 0x20 )
    {
        ch &= 0x1F;
        stop = src + 2;
    }
	
    /* 3 byte */
    else if ( c >= 0x10 )
    {
        ch &= 0xF;
        stop = src + 3;
    }
	
    /* 4 byte */
    else if ( c >= 8 )
    {
        ch &= 7;
        stop = src + 4;
    }
	
    /* 5 byte */
    else if ( c >= 4 )
    {
        ch &= 3;
        stop = src + 5;
    }
	
    /* illegal */
    else if ( c < 2 )
        return -1;
    
    /* 6 byte */
    else
    {
        ch &= 1;
        stop = src + 6;
    }
    
    /* must have sufficient input */
    if ( stop > end )
        return 0;
	
    /* complete the character */
    while ( ++ src != stop )
    {
        c = src [ 0 ] & 0x7F;
        if ( src [ 0 ] >= 0 || c >= 0x40 )
            return -1;
        ch = ( ch << 6 ) | c;
    }
	
    /* record the character */
    dst [ 0 ] = ch;
	
    /* return the bytes consumed */
    return ( int ) ( src - begin );
}

/* tracedb_utf32_utf8
 *  converts a single UTF32 character to UTF8 text
 *  returns the number of UTF8 bytes generated, such that:
 *    return > 0 means success
 *    return == 0 means insufficient output
 *    return < 0 means bad character or bad argument
 */
int tracedb_utf32_utf8 ( char *begin, char *end, unsigned int ch )
{
    int len;
    char *dst;
    unsigned int mask;

    if ( begin == NULL || end == NULL )
        return -1;
    if ( begin >= end )
        return 0;

    if ( ch < 128 )
    {
        begin [ 0 ] = ( char ) ch;
        return 1;
    }

    /* 2 byte */
    if ( ch < 0x00000800 )
    {
        /* 110xxxxx */
        mask = 0xC0U;
        len = 2;
    }
	
    /* 3 byte */
    else if ( ch < 0x00010000 )
    {
        /* 1110xxxx */
        mask = 0xE0U;
        len = 3;
    }

    /* 4 byte */
    else if ( ch < 0x00200000 )
    {
        /* 11110xxx */
        mask = 0xF0U;
        len = 4;
    }
	
    /* 5 byte */
    else if ( ch < 0x04000000 )
    {
        /* 111110xx */
        mask = 0xF8U;
        len = 5;
    }
	
    /* 6 byte */
    else
    {
        /* 1111110x */
        mask = 0xFCU;
        len = 6;
    }
	
    dst = begin + len;
    if ( dst > end )
        return 0;
    
    while ( -- dst > begin )
    {
        /* 10xxxxxx */
        dst [ 0 ] = ( char ) 0x80 | ( ( char ) ch & 0x3F );
        ch >>= 6;
    }
    
    dst [ 0 ] = ( char ) ( mask | ch );

    return len;
}

/* tracedb_utf16_string_size/len/measure
 *  measures UTF-16 strings
 */
size_t tracedb_utf16_string_size ( const uint16_t *str )
{
    unsigned int i, ch;

    for ( ch = str [ i = 0 ]; ch != 0 ; ch = str [ ++ i ] )
    {
        char ignore [ 8 ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    return i * sizeof * str;
}

unsigned int tracedb_utf16_string_len ( const uint16_t *str, size_t size )
{
    unsigned int i, str_len = ( unsigned int ) ( size >> 1 );

    for ( i = 0; i < str_len; ++ i )
    {
        char ignore [ 8 ];
        unsigned int ch = str [ i ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    return i;
}

unsigned int tracedb_utf16_string_measure ( const uint16_t *str, size_t *size )
{
    unsigned int i, ch;

    for ( ch = str [ i = 0 ]; ch != 0 ; ch = str [ ++ i ] )
    {
        char ignore [ 8 ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    * size = i * sizeof * str;

    return i;
}

/* tracedb_utf32_string_size/len/measure
 */
size_t tracedb_utf32_string_size ( const uint32_t *str )
{
    unsigned int i, ch;

    for ( ch = str [ i = 0 ]; ch != 0 ; ch = str [ ++ i ] )
    {
        char ignore [ 8 ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    return i * sizeof * str;
}

unsigned int tracedb_utf32_string_len ( const uint32_t *str, size_t size )
{
    unsigned int i, str_len = ( unsigned int ) ( size >> 2 );

    for ( i = 0; i < str_len; ++ i )
    {
        char ignore [ 8 ];
        unsigned int ch = str [ i ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    return i;
}

unsigned int tracedb_utf32_string_measure ( const uint32_t *str, size_t *size )
{
    unsigned int i, ch;

    for ( ch = str [ i = 0 ]; ch != 0 ; ch = str [ ++ i ] )
    {
        char ignore [ 8 ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    * size = i * sizeof * str;

    return i;
}

/* whcar_string_size/len/measure
 *  measures whcar_t strings
 */
size_t tracedb_wchar_string_size ( const wchar_t *str )
{
    unsigned int i, ch;

    for ( ch = str [ i = 0 ]; ch != 0 ; ch = str [ ++ i ] )
    {
        char ignore [ 8 ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    return i * sizeof * str;
}

unsigned int tracedb_wchar_string_len ( const wchar_t *str, size_t size )
{
    unsigned int i, str_len = ( unsigned int ) ( size / sizeof * str );

    for ( i = 0; i < str_len; ++ i )
    {
        char ignore [ 8 ];
        unsigned int ch = str [ i ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    return i;
}

unsigned int tracedb_wchar_string_measure ( const wchar_t *str, size_t *size )
{
    unsigned int i, ch;

    for ( ch = str [ i = 0 ]; ch != 0 ; ch = str [ ++ i ] )
    {
        char ignore [ 8 ];
        if ( tracedb_utf32_utf8 ( ignore, & ignore [ sizeof ignore ], ch ) <= 0 )
            break;
    }

    * size = i * sizeof * str;

    return i;
}
