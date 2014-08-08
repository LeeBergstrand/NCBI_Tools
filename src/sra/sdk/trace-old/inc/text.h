#ifndef _h_text_
#define _h_text_

#ifndef _h_itypes_
#include "itypes.h"
#endif

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * tracedb_String
 *  pseudo-intrinsic string
 */
typedef struct tracedb_String tracedb_String;
struct tracedb_String
{
    const char *addr;
    size_t size;
    unsigned int len;
};

/* tracedb_StringInit
 *  initializes a tracedb_String object
 */
#define tracedb_StringInit( s, val, sz, length ) \
    ( void ) ( ( s ) -> addr = ( val ), ( s ) -> size = ( sz ), ( s ) -> len = ( length ) )

#define tracedb_StringInitCtracedb_String( s, cstr ) \
    ( void ) ( ( s ) -> len = tracedb_string_measure ( ( s ) -> addr = ( cstr ), & ( s ) -> size ) )

/* CONST_STRING
 *  initialize a string from a manifest constant
 */
#define TRACEDB_CONST_STRING( s, val ) \
    tracedb_StringInit ( s, val, sizeof val - 1, sizeof val - 1 )

/* tracedb_StringSize
 *  size of string in bytes
 */
#define tracedb_StringSize( s ) \
    ( s ) -> size

/* tracedb_StringLength
 *  length of string in characters
 */
#define tracedb_StringLength( s ) \
    ( s ) -> len

/* tracedb_StringCopy
 *  allocates a copy of a string
 */
int tracedb_StringCopy ( const tracedb_String **cpy, const tracedb_String *str );

/* tracedb_StringConcat
 *  concatenate one string onto another
 */
int tracedb_StringConcat ( const tracedb_String **cat, const tracedb_String *a, const tracedb_String *b );

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
    unsigned int idx, unsigned int len );

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
int tracedb_StringHead ( const tracedb_String *str, unsigned int *ch );

/* tracedb_StringPopHead
 *  remove and return the first character
 *
 *  returns EINVAL if the character is bad,
 *  or ENODATA if the string is empty
 */
int tracedb_StringPopHead ( tracedb_String *str, unsigned int *ch );

/* tracedb_StringEqual
 *  compare strings for equality
 */
#define tracedb_StringEqual( a, b ) \
    ( ( a ) -> len == ( b ) -> len && \
    memcmp ( ( a ) -> addr, ( b ) -> addr, ( a ) -> len ) == 0 )

/* tracedb_StringCompare
 *  compare strings for relative ordering
 */
int tracedb_StringCompare ( const tracedb_String *a, const tracedb_String *b );

/* tracedb_StringCaseEqual
 *  compare strings for case-insensitive equality
 */
bool tracedb_StringCaseEqual ( const tracedb_String *a, const tracedb_String *b );

/* tracedb_StringCaseCompare
 *  compare strings for relative case-insensitive ordering
 */
int tracedb_StringCaseCompare ( const tracedb_String *a, const tracedb_String *b );

/* tracedb_StringMatch
 *  creates a substring of "a" in "match"
 *  for all of the sequential matching characters between "a" and "b"
 *  starting from character [ 0 ].
 *
 *  returns the number of characters that match.
 */
unsigned int tracedb_StringMatch ( tracedb_String *match, const tracedb_String *a, const tracedb_String *b );

/* tracedb_StringMatchExtend
 *  extends a substring of "a" in "match"
 *  for all of the sequential matching characters between "a" and "b"
 *  starting from character [ match -> len ].
 *
 *  returns the number of matching characters that were extended.
 */
unsigned int tracedb_StringMatchExtend ( tracedb_String *match, const tracedb_String *a, const tracedb_String *b );

/* tracedb_StringHash
 *  hash value for string
 */
#define tracedb_StringHash( s ) \
    tracedb_string_hash ( ( s ) -> addr, ( s ) -> size )

/* tracedb_StringCopyUTF...
 *  creates a tracedb_String from UTF16 or UTF32 UNICODE input
 *  wchar_t is one or the other, depending upon OS and compiler.
 */
int tracedb_StringCopyUTF16 ( const tracedb_String **cpy, const uint16_t *text, size_t bytes );
int tracedb_StringCopyUTF32 ( const tracedb_String **cpy, const uint32_t *text, size_t bytes );
int tracedb_StringCopyWChar_t ( const tracedb_String **cpy, const wchar_t *text, size_t bytes );

/* tracedb_StringWhack
 *  deallocates a string
 */
#define tracedb_StringWhack( s ) \
    ( ( s ) != NULL && ( s ) -> addr == ( const char* ) ( ( s ) + 1 ) ? \
      free ( s ) : ( void ) 0 )


/*--------------------------------------------------------------------------
 * raw text strings
 *  the internal representation of text strings is implementation
 *  dependent. it is assumed to be ASCII-7 or UTF-8, although
 *  this is determined by the implementation library of these functions.
 */

/* string_size
 *  length of string in bytes
 */
size_t tracedb_string_size ( const char *str );

/* string_len
 *  length of string in characters, when the size is known
 */
unsigned int tracedb_string_len ( const char *str, size_t size );

/* string_measure
 *  measures length of string in both characters and bytes
 */
unsigned int tracedb_string_measure ( const char *str, size_t *size );

/* string_copy
 *  copies whole character text into a buffer
 *  terminates with null byte if possible
 *  returns the number of bytes copied
 */
size_t tracedb_string_copy ( char *dst, size_t dst_size, const char *src, size_t src_size );

/* string_copy_measure
 *  copies whole character text into a buffer
 *  terminates with null byte if possible
 *  returns the number of bytes copied
 */
size_t tracedb_string_copy_measure ( char *dst, size_t dst_size, const char *src );

/* string_printf
 *  much like snprintf except that it handles overflows
 *
 *  returns the number of bytes written in num_writ on success,
 *  or the required buffer size ( including null byte ) on ENOBUFS
 *
 *  return value is an error code
 */
int tracedb_string_printf ( char *dst, size_t bsize, size_t *num_writ, const char *fmt, ... );
int tracedb_string_vprintf ( char *dst, size_t bsize, size_t *num_writ, const char *fmt, va_list args );

/* tolower_copy
 *  copies whole character text in lower-case
 *  terminates with null byte if possible
 *  returns the number of bytes copied
 */
size_t tracedb_tolower_copy ( char *dst, size_t dst_size, const char *src, size_t src_size );

/* toupper_copy
 *  copies whole character text in upper-case
 *  terminates with null byte if possible
 *  returns the number of bytes copied
 */
size_t tracedb_toupper_copy ( char *dst, size_t dst_size, const char *src, size_t src_size );

/* string_cmp
 *  performs a safe strncmp
 *
 *  "max_chars" limits the extent of the comparison
 *  to not exceed supplied value, i.e. the number of
 *  characters actually compared will be the minimum
 *  of asize, bsize and max_chars.
 */
int tracedb_string_cmp ( const char *a, size_t asize,
    const char *b, size_t bsize, unsigned int max_chars );

/* strcase_cmp
 *  like string_cmp except case insensitive
 */
int tracedb_strcase_cmp ( const char *a, size_t asize,
    const char *b, size_t bsize, unsigned int max_chars );

/* string_match
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
    const char *b, size_t bsize, unsigned int max_chars, size_t *msize );

/* strcase_match
 *  like string_match except case insensitive
 */
unsigned int tracedb_strcase_match ( const char *a, size_t asize,
    const char *b, size_t bsize, unsigned int max_chars, size_t *msize );

/* string_chr
 *  performs a safe strchr
 *  "ch" is in UTF32
 */
char *tracedb_string_chr ( const char *str, size_t size, unsigned int ch );

/* string_rchr
 *  performs a safe strrchr
 */
char *tracedb_string_rchr ( const char *str, size_t size, unsigned int ch );

/* string_brk
 *  performs a safe strpbrk
 */
char *tracedb_string_brk ( const char *str, size_t size,
    const char *accept, size_t asize );

/* string_rbrk
 */
char *tracedb_string_rbrk ( const char *str, size_t size,
    const char *accept, size_t asize );

/* string_hash
 *  hashes a string
 */
unsigned int tracedb_string_hash ( const char *str, size_t size );

/* string_idx
 *  seek an indexed character
 *
 *  the efficiency is based upon chosen internal
 *  string representation, which, when using single byte chars,
 *  is simple and efficient.
 *
 *  on the other hand, UTF-8 has a variable character width,
 *  requiring scanning of the entire string until the indexed
 *  character is found.
 */
char *tracedb_string_idx ( const char *str, size_t size, unsigned int idx );


/*--------------------------------------------------------------------------
 * conversion between UTF-32 and UTF-8 UNICODE
 */

/* tracedb_utf8_utf32
 *  converts UTF-8 text to a single UTF-32 character
 *  returns the number of UTF8 bytes consumed, such that:
 *    return > 0 means success
 *    return == 0 means insufficient input
 *    return < 0 means bad input
 */
int tracedb_utf8_utf32 ( unsigned int *ch, const char *begin, const char *end );

/* tracedb_utf32_utf8
 *  converts a single UTF-32 character to UTF-8 text
 *  returns the number of UTF8 bytes generated, such that:
 *    return > 0 means success
 *    return == 0 means insufficient output
 *    return < 0 means bad character
 */
int tracedb_utf32_utf8 ( char *begin, char *end, unsigned int ch );


/*--------------------------------------------------------------------------
 * support for 16 and 32-bit UTF formats
 */

/* tracedb_utf16_string_size/len/measure
 *  measures UTF-16 strings
 */
size_t tracedb_utf16_string_size ( const uint16_t *str );
unsigned int tracedb_utf16_string_len ( const uint16_t *str, size_t size );
unsigned int tracedb_utf16_string_measure ( const uint16_t *str, size_t *size );

/* tracedb_utf32_string_size/len/measure
 */
size_t tracedb_utf32_string_size ( const uint32_t *str );
unsigned int tracedb_utf32_string_len ( const uint32_t *str, size_t size );
unsigned int tracedb_utf32_string_measure ( const uint32_t *str, size_t *size );

/* tracedb_wchar_string_size/len/measure
 *  measures wchar_t strings
 */
size_t tracedb_wchar_string_size ( const wchar_t *str );
unsigned int tracedb_wchar_string_len ( const wchar_t *str, size_t size );
unsigned int tracedb_wchar_string_measure ( const wchar_t *str, size_t *size );

/* conversion from UTF-16 to internal standard */
unsigned int tracedb_utf16_cvt_string_len ( const uint16_t *src,
    size_t src_size, size_t *dst_size );
unsigned int tracedb_utf16_cvt_string_measure ( const uint16_t *src,
    size_t *src_size, size_t *dst_size );
size_t tracedb_utf16_cvt_string_copy ( char *dst, size_t dst_size,
    const uint16_t *src, size_t src_size );

/* conversion from UTF-32 to internal standard */
unsigned int tracedb_utf32_cvt_string_len ( const uint32_t *src,
    size_t src_size, size_t *dst_size );
unsigned int tracedb_utf32_cvt_string_measure ( const uint32_t *src,
    size_t *src_size, size_t *dst_size );
size_t tracedb_utf32_cvt_string_copy ( char *dst, size_t dst_size,
    const uint32_t *src, size_t src_size );

/* conversion from wchar_t to internal standard */
unsigned int tracedb_wchar_cvt_string_len ( const wchar_t *src,
    size_t src_size, size_t *dst_size );
unsigned int tracedb_wchar_cvt_string_measure ( const wchar_t *src,
    size_t *src_size, size_t *dst_size );
size_t tracedb_wchar_cvt_string_copy ( char *dst, size_t dst_size,
    const wchar_t *src, size_t src_size );

/*--------------------------------------------------------------------------
 * support for ISO-8859-x 8-bit character sets
 */

/* iso8859_utf32
 *  converts 8-bit text to a single UTF-32 character
 *  returns the number of 8-bit bytes consumed, such that:
 *    return > 0 means success
 *    return == 0 means insufficient input
 *    return < 0 means bad input
 */
int tracedb_iso8859_utf32 ( const unsigned int map [ 128 ],
    unsigned int *ch, const char *begin, const char *end );

/* iso8859_string_size/len/measure
 *  measures UTF-16 strings
 */
size_t tracedb_iso8859_string_size ( const unsigned int map [ 128 ],
    const char *str );
unsigned int tracedb_iso8859_string_len ( const unsigned int map [ 128 ],
    const char *str, size_t size );
unsigned int tracedb_iso8859_string_measure ( const unsigned int map [ 128 ],
    const char *str, size_t *size );

/* conversion from ISO-8859-x to internal standard */
unsigned int tracedb_iso8859_cvt_string_len ( const unsigned int map [ 128 ],
    const char *src, size_t src_size, size_t *dst_size );
unsigned int tracedb_iso8859_cvt_string_measure ( const unsigned int map [ 128 ],
    const char *src, size_t *src_size, size_t *dst_size );
size_t tracedb_iso8859_cvt_string_copy ( const unsigned int map [ 128 ],
    char *dst, size_t dst_size, const char *src, size_t src_size );

/* some externally defined character maps */
extern const unsigned int tracedb_iso8859_1 [ 128 ];
extern const unsigned int tracedb_cp1252 [ 128 ];

#ifdef __cplusplus
}
#endif

#endif /* _h_text_ */
