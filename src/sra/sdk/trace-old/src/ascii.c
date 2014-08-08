#include "text.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/*--------------------------------------------------------------------------
 * raw text strings
 */

/* tracedb_string_len
 *  length of string in characters
 */
unsigned int tracedb_string_len ( const char *str, size_t size )
{
    assert ( str != NULL );
    return ( unsigned int ) size;
}

/* tracedb_string_measure
 *  measures length of string in both characters and bytes
 */
unsigned int tracedb_string_measure ( const char *str, size_t *size )
{
    size_t len;
    assert ( str != NULL );
    len = strlen ( str );
    if ( size != NULL )
        * size = len;
    return ( unsigned int ) len;
}

/* tracedb_string_copy
 *  copies whole character text into a buffer
 *  terminates with null byte if possible
 *  returns the number of bytes copied
 */
size_t tracedb_string_copy ( char *dst, size_t dst_size, const char *src, size_t src_size )
{
    assert ( dst != NULL && src != NULL );
    if ( dst_size < src_size )
        src_size = dst_size;
    memcpy ( dst, src, src_size );
    if ( dst_size > src_size )
        dst [ src_size ] = 0;
    return src_size;
}

/* tracedb_string_copy_measure
 *  copies whole character text into a buffer
 *  terminates with null byte if possible
 *  returns the number of bytes copied
 */
size_t tracedb_string_copy_measure ( char *dst, size_t dst_size, const char *src )
{
    size_t i;

    assert ( dst != NULL && src != NULL );

    /* this type of coding is meant for vectorization */
    for ( i = 0; i < dst_size && src [ i ] != 0; ++ i )
        dst [ i ] = src [ i ];

    /* attempt to null terminate */
    if ( i < dst_size )
        dst [ i ] = 0;

    return i;
}

/* tolower_copy
 *  copies whole character text in lower-case
 *  terminates with null byte if possible
 *  returns the number of bytes copied
 */
size_t tracedb_tolower_copy ( char *dst, size_t dst_size, const char *src, size_t src_size )
{
    size_t i;
    assert ( dst != NULL && src != NULL );
    if ( dst_size < src_size )
        src_size = dst_size;
    for ( i = 0; i < src_size; ++ i )
        dst [ i ] = ( char ) tolower ( src [ i ] );
    if ( dst_size > src_size )
        dst [ src_size ] = 0;
    return src_size;
}

/* toupper_copy
 *  copies whole character text in upper-case
 *  terminates with null byte if possible
 *  returns the number of bytes copied
 */
size_t tracedb_toupper_copy ( char *dst, size_t dst_size, const char *src, size_t src_size )
{
    size_t i;
    assert ( dst != NULL && src != NULL );
    if ( dst_size < src_size )
        src_size = dst_size;
    for ( i = 0; i < src_size; ++ i )
        dst [ i ] = ( char ) toupper ( src [ i ] );
    if ( dst_size > src_size )
        dst [ src_size ] = 0;
    return src_size;
}

/* tracedb_string_cmp
 *  performs a safe strncmp
 *
 *  "max_chars" limits the extent of the comparison
 *  to not exceed supplied value, i.e. the number of
 *  characters actually compared will be the minimum
 *  of asize, bsize and max_chars.
 */
int tracedb_string_cmp ( const char *a, size_t asize,
    const char *b, size_t bsize, unsigned int max_chars )
{
    assert ( a != NULL && b != NULL );
    if ( ( size_t ) max_chars > asize )
        max_chars = ( unsigned int ) asize;
    if ( ( size_t ) max_chars > bsize )
        max_chars = ( unsigned int ) bsize;
    return strncmp ( a, b, max_chars );
}

/* strcase_cmp
 *  like string_cmp except case insensitive
 */
int tracedb_strcase_cmp ( const char *a, size_t asize,
    const char *b, size_t bsize, unsigned int max_chars )
{
    assert ( a != NULL && b != NULL );
    if ( ( size_t ) max_chars > asize )
        max_chars = ( unsigned int ) asize;
    if ( ( size_t ) max_chars > bsize )
        max_chars = ( unsigned int ) bsize;
    return strncasecmp ( a, b, max_chars );
}

/* tracedb_string_match
 *  returns the number of matching characters
 *
 *  "max_chars" limits the extent of the comparison
 *  to not exceed supplied value, i.e. the number of
 *  characters actually compared will be the minimum
 *  of asize, bsize and max_chars.
 *
 *  "msize" will be set to the size of the matched string
 *  if not NULL
 */
unsigned int tracedb_string_match ( const char *a, size_t asize,
    const char *b, size_t bsize, unsigned int max_chars, size_t *msize )
{
    unsigned int i;

    assert ( a != NULL && b != NULL );
    if ( ( size_t ) max_chars > asize )
        max_chars = ( unsigned int ) asize;
    if ( ( size_t ) max_chars > bsize )
        max_chars = ( unsigned int ) bsize;

    for ( i = 0; i < max_chars; ++ i )
    {
        if ( a [ i ] != b [ i ] )
        {
            if ( msize != NULL )
                * msize = ( size_t ) i;
            return i;
        }
    }

    if ( msize != NULL )
        * msize = ( size_t ) max_chars;
    return max_chars;
}

/* strcase_match
 *  like string_match except case insensitive
 */
unsigned int tracedb_strcase_match ( const char *a, size_t asize,
    const char *b, size_t bsize, unsigned int max_chars, size_t *msize )
{
    unsigned int i;

    assert ( a != NULL && b != NULL );
    if ( ( size_t ) max_chars > asize )
        max_chars = ( unsigned int ) asize;
    if ( ( size_t ) max_chars > bsize )
        max_chars = ( unsigned int ) bsize;

    for ( i = 0; i < max_chars; ++ i )
    {
        if ( a [ i ] != b [ i ] && tolower ( a [ i ] ) != tolower ( b [ i ] ) )
        {
            if ( msize != NULL )
                * msize = ( size_t ) i;
            return i;
        }
    }

    if ( msize != NULL )
        * msize = ( size_t ) max_chars;
    return max_chars;
}

/* string_chr
 *  performs a safe strchr
 *  "ch" is in UTF32
 */
char *tracedb_string_chr ( const char *str, size_t size, unsigned int ch )
{
    size_t i;
    
    if ( str == NULL || ch >= 128 )
        return NULL;
    
    for ( i = 0; i < size; ++ i )
    {
        if ( str [ i ] == ( char ) ch )
            return ( char* ) str + i;
    }
    return NULL;
}

/* string_rchr
 *  performs a safe strrchr
 */
char *tracedb_string_rchr ( const char *str, size_t size, unsigned int ch )
{
    int i;
    
    if ( str == NULL || ch >= 128 )
        return NULL;
    
    for ( i = ( int ) size; -- i >= 0; )
    {
        if ( str [ i ] == ( char ) ch )
            return ( char* ) str + i;
    }
    return NULL;
}

/* string_brk
 *  performs a safe strpbrk
 */
char *tracedb_string_brk ( const char *str, size_t size,
    const char *accept, size_t asize )
{
    int i;
    bool brk [ 128 ];

    memset ( brk, 0, sizeof brk );

    for ( i = 0; i < ( int ) asize; ++ i )
    {
        if ( accept [ i ] >= 0 )
            brk [ ( int ) accept [ i ] ] = true;
    }

    for ( i = 0; i < ( int ) size; ++ i )
    {
        if ( str [ i ] >= 0 && brk [ ( int ) str [ i ] ] )
            return ( char* ) str + i;
    }
    return NULL;
}

/* string_rbrk
 */
char *tracedb_string_rbrk ( const char *str, size_t size,
    const char *accept, size_t asize )
{
    int i;
    bool brk [ 128 ];

    memset ( brk, 0, sizeof brk );

    for ( i = 0; i < ( int ) asize; ++ i )
    {
        if ( accept [ i ] >= 0 )
            brk [ ( int ) accept [ i ] ] = true;
    }

    for ( i = ( int ) size; -- i >= 0; )
    {
        if ( str [ i ] >= 0 && brk [ ( int ) str [ i ] ] )
            return ( char* ) str + i;
    }
    return NULL;
}

/* string_idx
 *  seek an indexed character
 */
char *tracedb_string_idx ( const char *str, size_t size, unsigned int idx )
{
    assert ( str != NULL );
    if ( idx >= size )
        return NULL;
    return ( char* ) str + idx;
}

/* conversion from UTF-16 to internal standard */
unsigned int tracedb_utf16_cvt_string_len ( const uint16_t *src,
    size_t src_size, size_t *dst_size )
{
    /* ASCII never rejects a valid UNICODE character */
    unsigned int len = tracedb_utf16_string_len ( src, src_size );
    * dst_size = ( size_t ) len;
    return len;
}

unsigned int tracedb_utf16_cvt_string_measure ( const uint16_t *src,
    size_t *src_size, size_t *dst_size )
{
    unsigned int len = tracedb_utf16_string_measure ( src, src_size );
    * dst_size = ( size_t ) len;
    return len;
}

size_t tracedb_utf16_cvt_string_copy ( char *dst, size_t dst_size,
    const uint16_t *src, size_t src_size )
{
    char *begin = dst;
    char *dend = dst + dst_size;
    const uint16_t *send = ( const uint16_t* ) ( ( const char* ) src + src_size );
    
    while ( dst < dend && src < send )
    {
        unsigned int ch = * src ++;
        * dst ++ = ( ch >= 128 ) ? '.' : ( char ) ch;
    }
    
    if ( dst < dend )
        * dst = 0;

    return ( size_t ) ( dst - begin );
}

/* conversion from UTF-32 to internal standard */
unsigned int tracedb_utf32_cvt_string_len ( const uint32_t *src,
    size_t src_size, size_t *dst_size )
{
    unsigned int len = tracedb_utf32_string_len ( src, src_size );
    * dst_size = ( size_t ) len;
    return len;
}

unsigned int tracedb_utf32_cvt_string_measure ( const uint32_t *src,
    size_t *src_size, size_t *dst_size )
{
    unsigned int len = tracedb_utf32_string_measure ( src, src_size );
    * dst_size = ( size_t ) len;
    return len;
}

size_t tracedb_utf32_cvt_string_copy ( char *dst, size_t dst_size,
    const uint32_t *src, size_t src_size )
{
    char *begin = dst;
    char *dend = dst + dst_size;
    const uint32_t *send = ( const uint32_t* ) ( ( const char* ) src + src_size );
    
    while ( dst < dend && src < send )
    {
        unsigned int ch = * src ++;
        * dst ++ = ( ch >= 128 ) ? '.' : ( char ) ch;
    }
    
    if ( dst < dend )
        * dst = 0;

    return ( size_t ) ( dst - begin );
}

/* conversion from wchar_t to internal standard */
unsigned int tracedb_wchar_cvt_string_len ( const wchar_t *src,
    size_t src_size, size_t *dst_size )
{
    unsigned int len = tracedb_wchar_string_len ( src, src_size );
    * dst_size = ( size_t ) len;
    return len;
}

unsigned int tracedb_wchar_cvt_string_measure ( const wchar_t *src,
    size_t *src_size, size_t *dst_size )
{
    unsigned int len = tracedb_wchar_string_measure ( src, src_size );
    * dst_size = ( size_t ) len;
    return len;
}

size_t tracedb_wchar_cvt_string_copy ( char *dst, size_t dst_size,
    const wchar_t *src, size_t src_size )
{
    char *begin = dst;
    char *dend = dst + dst_size;
    const wchar_t *send = ( const wchar_t* ) ( ( const char* ) src + src_size );
    
    while ( dst < dend && src < send )
    {
        unsigned int ch = * src ++;
        * dst ++ = ( ch >= 128 ) ? '.' : ( char ) ch;
    }
    
    if ( dst < dend )
        * dst = 0;

    return ( size_t ) ( dst - begin );
}
