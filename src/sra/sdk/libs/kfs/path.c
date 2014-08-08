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

#include <kfs/extern.h>
#include <os-native.h>
#include <kfs/path.h>
#include "path-priv.h"
#include <kfs/kfs-priv.h>

#include <klib/refcount.h>
#include <klib/text.h>
#include <klib/printf.h>
#include <klib/log.h>
#include <klib/debug.h>
#include <klib/rc.h>

#include <stdlib.h>
#include <string.h>
/* #include <sysalloc.h> */
#include <ctype.h>
#include <assert.h>

rc_t KPOptionMake (KPOption ** new_obj, KPOption_t name, const char * value, size_t size)
{
    KPOption * obj;

    assert (new_obj);

    obj = malloc (sizeof (*obj));
    if (obj == NULL)
        return RC (rcFS, rcPath, rcConstructing, rcMemory, rcExhausted);

    obj->name = name;
    StringInit (&obj->value, value, size, string_len (value, size));
    *new_obj = obj;
    return 0;
}

void CC KPOptionWhack (BSTNode * self, void * ignored)
{
    free (self);
}

int CC KPOptionCmp ( const void * item, const BSTNode * n )
{
    KPOption_t o = (KPOption_t)(size_t)item;
    const KPOption * b = (const KPOption *)n;

    return o - b->name;
}

int CC KPOptionSort ( const BSTNode * item, const BSTNode * n )
{
    const KPOption * a = (const KPOption *)item;
    const KPOption * b = (const KPOption *)n;

    return a->name - b->name;
}


#if 0
static
rc_t KPathMakeCanon (KPath * self)
{
    char * low;  /* 'root' location in path */
    char * dst;  /* target reference for removing . and .. nodes */
    char * last; /* '/' at end of previous node */
    char * lim;  /* '\0' at end of the path string coming in */


    end = self->path.addr + self->path.size;
    low = self->path.addr;
    if (low[1] == '/') /* path starts with // which we allow in windows */
        ++low;
    dst = last = low;

    for (;;)
    {
        char * src; /* '/' or '\0' at end of this path node */

        src = strchr (prev + 1, '/');
        if (src == NULL)
            src = lim;

        /* detect special sequences based on length of node */
        switch (src-last)
        {
        case 1: /* an empty node name - only allowed in the beginning */
            last = src;  /* skip over */
            if (src != lim)
                continue;
            break;
        case 2:
            if (last[1] == '.')
            {
                last = src;  /* skip over */
                if (src != lim)
                    continue;
            }
            break;

        case 3:
            if ((last[1] == '.') && (last[2] == '.'))
            {
                /* remove previous node name */
                dst[0] = '\0';
                dst = strrchr (low, '/');
                if ((dst == NULL) || (dst < low))
                    return RC (rcFS, rcPath, rcAccessing, rcPath, rcInvalid);

                last = src;
                if (src != lim)
                    continue;
            }
            break;
        }

        assert (src >= last);
        if (dst != last)
            memmove (dst, last, src-last);
    }
}
#endif
/* Destroy
 */
rc_t KPathDestroy (const KPath * cself)
{
    if (cself)
    {
        KPath * self = (KPath*)cself;
        PATH_DEBUG (("-----\n%s: %p %p\n\n", __func__, cself, cself->storage ));
        BSTreeWhack (&self->options, KPOptionWhack, NULL);
        free (self->storage);
        free (self);
    }
    return 0;
}


static const char class_name[] = "KPath";

/* AddRef
 *  creates a new reference
 *  ignores NULL references
 */
LIB_EXPORT rc_t CC KPathAddRef ( const KPath *self )
{
    if ( self != NULL )
    {
        switch (KRefcountAdd (&self->refcount, class_name))
        {
        case krefLimit:
        case krefNegative:
            return RC (rcFS, rcPath, rcAttaching, rcRange, rcInvalid);
        }
    }
    return 0;
}


/* Release
 *  discard reference to file
 *  ignores NULL references
 */
LIB_EXPORT rc_t CC KPathRelease ( const KPath *self )
{
    if ( self != NULL )
    {
        switch (KRefcountDrop (&self->refcount, class_name))
        {
        case krefWhack:
            KPathDestroy (self);
            break;
        case krefLimit:
        case krefNegative:
            return RC (rcFS, rcPath, rcReleasing, rcRange, rcInvalid);
        }
    }
    return 0;
}


#if USE_EXPERIMENTAL_CODE
/* not fully reselient to bad input */
static
char decode_nibble (char c)
{
    if ((c >= '0') && (c <= '9'))
        return (c - '0');
    if ((c >= 'a') && (c <= 'z'))
        return (c - 'a');
    if ((c >= 'A') && (c <= 'Z'))
        return (c - 'A');
    return (0);
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool string_decode (char * p)
{
    char * q;
    size_t limit;
    size_t ix;

    q = p;
    limit = string_size (p);

    for (ix = 0; ix < limit; ++ix)
    {
        if (p[0] == '%')
        {
            if ((ix + 2 > limit) || ! isxdigit (p[1]) || ! isxdigit (p[2]))
                return false;
            *q = decode_nibble (p[1]) << 4;
            *q += decode_nibble (p[2]);
        }
        else if (q != p)
            *q = *p;
        ++p;
        ++q;
    }
    if (p != q)
        *q = '\0';
    return true;
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
rc_t StringDecode (String * self)
{
    size_t limit;
    size_t ix;
    char * p;
    char * q;

    p = q = (char *)self->addr;
    limit = self->size;
    for (ix = 0; ix < limit; ++ix)
    {
        if (p[0] == '%')
        {
            if ((ix + 2 > limit) || ! isxdigit (p[1]) || ! isxdigit (p[2]))
                return RC (rcFS, rcPath, rcDecoding, rcPath, rcInvalid);
            *q = decode_nibble (p[1]) << 4;
            *q += decode_nibble (p[2]);
        }
        else if (q != p)
            *q = *p;
        ++p;
        ++q;
    }
    if (p != q)
    {
        *q = '\0';
        self->size = q - self->addr;
        self->len = string_len (self->addr, self->size);
    }
    return 0;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_sub_delim (int ch)
{
    switch (ch)
    {
    case '!': case '$': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case ';': case '=':
        return true;
    default:
        return false;
    }
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
bool is_gen_delim (int ch)
{
    switch (ch)
    {
    case '"': case '/': case '?': case '#': case '[': case ']': case '@':
        return true;
    default:
        return false;
    }
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
bool is_reserved (int ch)
{
    return is_gen_delim (ch) || is_sub_delim (ch);
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_unreserved (int ch)
{
    return (isalnum (ch) ||
            (ch == '-') ||
            (ch == '.') ||
            (ch == '_') ||
            (ch == '~'));
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_scheme_char (int ch)
{
    return (isalnum (ch) ||
            (ch == '+') ||
            (ch == '-') ||
            (ch == ','));
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_scheme (const char * str)
{
    if ( !isalpha (*str++))
        return false;
    while (*str)
        if ( ! is_scheme_char (*str++))
            return false;
    return true;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_iprivate (int ch )
{
    return (((ch >= 0x00E000) && (ch <= 0x00F8FF)) ||
            ((ch >= 0x0F0000) && (ch <= 0x0FFFFD)) ||
            ((ch >= 0x100000) && (ch <= 0x10FFFD)));
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_ucschar (int ch)
{
    return (((ch >= 0xA0)   && (ch <= 0xD7FF)) ||
            ((ch >= 0xF900) && (ch <= 0xFDCF)) ||
            ((ch >= 0xFDF0) && (ch <= 0xFFEF)) ||
            ((ch >= 0x10000) && (ch <= 0x1FFFD)) ||
            ((ch >= 0x20000) && (ch <= 0x2FFFD)) ||
            ((ch >= 0x30000) && (ch <= 0x3FFFD)) ||
            ((ch >= 0x40000) && (ch <= 0x4FFFD)) ||
            ((ch >= 0x50000) && (ch <= 0x5FFFD)) ||
            ((ch >= 0x60000) && (ch <= 0x6FFFD)) ||
            ((ch >= 0x70000) && (ch <= 0x7FFFD)) ||
            ((ch >= 0x80000) && (ch <= 0x8FFFD)) ||
            ((ch >= 0x90000) && (ch <= 0x9FFFD)) ||
            ((ch >= 0xA0000) && (ch <= 0xAFFFD)) ||
            ((ch >= 0xB0000) && (ch <= 0xBFFFD)) ||
            ((ch >= 0xC0000) && (ch <= 0xCFFFD)) ||
            ((ch >= 0xD0000) && (ch <= 0xDFFFD)) ||
            ((ch >= 0xE0000) && (ch <= 0xEFFFD)));
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_iunreserved (int ch)
{
    return is_unreserved (ch) || is_ucschar (ch);
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_ipchar (int ch)
{
    return is_iunreserved (ch) || is_sub_delim (ch) || 
        (ch == ':') || (ch == '@');
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_query (const char * str)
{
    for ( ; *str; ++str)
    {
        if (! is_ipchar (*str) &&
            ! is_iprivate (*str) &&
            (*str != '/') &&
            (*str != '?'))
        {
            return false;
        }
    }
    return true;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_fragment (const char * str)
{
    for ( ; *str; ++str)
    {
        if (! is_ipchar (*str) &&
            ! is_sub_delim (*str) &&
            (*str != '/') &&
            (*str != '?'))
        {
            return false;
        }
    }
    return true;
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
bool is_isegment (const char * str, size_t sz)
{
    const char * end = str + sz;
    size_t ix;
    int cnt;

    for (ix = 0; ix < sz; ix += cnt)
    {
        uint32_t ch;
        cnt = utf8_utf32 (&ch, str + ix, end);
        if (cnt <= 0)
            return false;
        if ( ! is_ipchar (ch))
            return false;
    }
    return true;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_iuserinfo (const char * str)
{
    for (;;++str)
    {
        if (is_iunreserved (*str))
            ;
        else if (is_sub_delim (*str))
            ;
        else if (*str != ':')
            break;
    }
    return str;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_ireg_name (const char * str)
{
    for (;;++str)
    {
        if (is_iunreserved (*str))
            ;
        else if (is_sub_delim (*str))
            ;
        else
            break;
    }
    return str;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_port (const char * str)
{
    while (isdigit (*str))
        ++str;
    return str;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_ihost (const char * str)
{
    /* not doing ip addresses yet */

    return eat_ireg_name (str);
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_iuserinfo_at (const char * str)
{
    const char * temp = eat_iuserinfo (str);
    if (temp == NULL)
        return temp;
    if (*++temp != '@')
        return NULL;
    return temp;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_iauthority (const char * str)
{
    const char * temp;

    temp = eat_iuserinfo_at (str);

    if (temp != NULL)
        str = temp;

    temp = eat_ihost (str);
    if (temp == NULL)
        return false;
    str = temp;

    if (*str == ':')
        return eat_port (str+1);

    return str;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_file_iauthority (const char * str)
{
    const char * temp;

    temp = eat_ihost (str);
    if (temp == NULL)
        return false;
    return temp;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_isegment (const char * str)
{
    while (is_ipchar (*str))
        ++str;
    return str;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_isegment_nz (const char * str)
{
    const char * temp = eat_isegment (str);
    if (temp == str)
        return NULL;
    return temp;
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
const char * eat_isegment_nz_nc (const char * str)
{
    const char * temp = str;

    while (is_iunreserved (*str) ||
           is_sub_delim (*str) ||
           (*str == '@'))
        ++str;
    if (temp == str)
        return false;
    return str;
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
bool is_isegment_nz (const char * str, size_t sz)
{
    if (sz == 0)
        return false;
    return is_isegment (str, sz);
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
bool is_isegment_nz_nc (const char * str, size_t sz)
{
    if (sz == 0)
        return false;
    if (string_chr (str, sz, ':') != NULL)
        return false;
    return is_isegment (str, sz);
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
bool is_ireg_name (const char * str, size_t sz)
{
    const char * end = str + sz;
    size_t ix;
    int cnt;

    for (ix = 0; ix < sz; ix += cnt)
    {
        uint32_t ch;
        cnt = utf8_utf32 (&ch, str + ix, end);
        if (cnt <= 0)
            return false;
        if ( ! is_iunreserved (ch) &&
             ! is_sub_delim (ch))
            return false;
    }
    return true;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_ipath_rootless (const char * str);
static
bool is_ipath_absolute (const char * str)
{
    if (*str++ != '/')
        return false;
    return is_ipath_rootless (str);
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_ipath_empty (const char * str)
{
    return (*str == '\0');
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_ipath_abempty (const char * str)
{
    for (;;)
    {
        if (is_ipath_empty (str))
            return true;

        else if (*str++ != '/')
            return false;

        else
        {
            const char * temp = eat_isegment (str);
            if (temp == NULL)
                return false;
            str = temp;
        }
    }
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_ipath_rootless (const char * str)
{
    str = eat_isegment_nz (str);
    if (str == NULL)
        return false;

    return is_ipath_abempty (str);
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
bool is_ipath_noscheme (const char * str)
{
    str = eat_isegment_nz_nc (str);

    if (str == NULL)
        return false;

    return is_ipath_abempty (str);
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_hier (const char * str)
{
    if ((str[0] == '/') && (str[1] == '/'))
    {
        const char * temp = eat_iauthority (str);
        if (temp != NULL)
            if (is_ipath_abempty (temp))
                return true;
    }
    if (is_ipath_absolute (str))
        return true;

    if (is_ipath_rootless (str))
        return true;

    if (is_ipath_empty (str))
        return true;

    return false;
}
#endif



#if USE_EXPERIMENTAL_CODE && 0
/*
 * confusion and ambiguity in the world of RFCs...
 *  We're gonna go with the RFC-3987 definition of ihost as ireg-name
 */
static
const char * eat_hostname (const char * str)
{
    /* -----
     * hostname = ireg_name
     * ireg_name = *(iunreserved / pct_encoded / sub_delim
     *
     * legal terminators are NUL or '/'
     */
    while (is_iunreserved (*str) &&
           is_sub_delim (*str))
        ++str;
    switch (*str)
    {
    case '\0':
    case '/':
        return str;
    default:
        return NULL;
    }
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
/* -----
 * number.number.number.numer 
 * does not range check number
 * sigh...
 */
static
const char * eat_ipv6 (const char * str)
{
#if 1
    return NULL; /* not gonna do them yet */
#else
    const char * temp;
    int digits;
    int colons;
    int doublecolons = 0;

    /* non empty first part */
    for (digits = 0; isxdigit (*str); ++str)
        ;
    if ((digits == 0) || (digits > 4))
        return NULL;

    if (*str++ != ':')
        return NULL;

    for (digits = 0; isxdigit (*str); ++str)
        ;
    if (digits == 0)
        doublecolons = 1;
    else if (digits > 4)
        return NULL;

    for (digits = 0; isxdigit (*str); ++str)
        ;
    if (digits == 0)
    {
        if (doublecolons > 0)
            return NULL;
        doublecolons = 1;
    }
    else if (digits > 4)
        return NULL;

    for (digits = 0; isxdigit (*str); ++str)
        ;
    if (digits == 0)
    {
        if (doublecolons > 0)
            return NULL;
        doublecolons = 1;
    }
    else if (digits > 4)
        return NULL;

/* ... eeek! */
    return NULL;
#endif
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
/* -----
 * number.number.number.numer 
 * does not range check number
 */
static
const char * eat_ipv4 (const char * str)
{
    do
        if (! isdigit(*str++))
            return NULL;
    while (*str != '.');
    ++str;
    do
        if (! isdigit(*str++))
            return NULL;
    while (*str != '.');
    ++str;
    do
        if (! isdigit(*str++))
            return NULL;
    while (*str != '.');
    ++str;
    do
        if (! isdigit(*str++))
            return NULL;
    while (isdigit (*str));
    return str;
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
const char * eat_hostnumber (const char * str)
{
    const char * temp;

    temp = eat_ipv4 (str);
    if (temp == NULL)
        temp = eat_ipv6 (str);
    return temp;
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
const char * eat_host (const char * str)
{
    const char * temp;

    temp = eat_hostnumber (str);
    if (temp == NULL)
        temp = eat_hostname (str);
    return temp;
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
static
const char * eat_fsegment (const char * str)
{
    for (;;++str)
    {
        if ((*str == '/') ||
            (*str == '\0'))
        {
            break;
        }
        else if ( ! is_ipchar (*str))
            return NULL;
    }
    return str;
}
#endif
#if USE_EXPERIMENTAL_CODE && 0
/* fpath = fsegment *[ "/" fsegment ] */
static
bool is_fpath (const char * str)
{
    for (;;)
    {
        str = eat_fsegment (str);
        if (str == NULL)
            return false;
        if (*str == '\0')
            return true;
        if (*str != '/')
            return false;
        ++str;
    }
    return false; /* unreachable really */
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_file_hier (const char * str)
{
#if 0
    const char * temp;
    /* 
     * we'll expect the "file://host/fpath"
     * but also accept the inccorect "file:/fpath"
     */
    if (*str++ != '/')
        return false;

    if (*str == '/') /* must be correct version */
    {
        ++str;
        temp = eat_host (str);
        if (temp == NULL)
            return false;
        str = temp;
        if (*str++ != '/')
            return false;
    }
    return is_fpath (str);
#else
    if ((str[0] == '/') && (str[1] == '/'))
    {
        const char * temp = eat_file_iauthority (str);
        if (temp != NULL)
            if (is_ipath_abempty (temp))
                return true;
    }
    if (is_ipath_absolute (str))
        return true;

    if (is_ipath_rootless (str))
        return true;

    if (is_ipath_empty (str))
        return true;

    return false;
#endif
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_file_query (const char * str)
{
    return (*str == '\0');

}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_file_fragment (const char * str)
{
    return (*str == '\0');
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_kfs_hier (const char * str)
{
    return is_file_hier (str);
}
#endif
#if USE_EXPERIMENTAL_CODE
static
const char * eat_kfs_query (const char * str, KPOption ** opt)
{
    /*
     *    query_entry = "encrypt" / "enc" / ( "pwfile=" hier-part ) / ( "pwfd=" fd )
     */
    assert (str);
    assert (opt);

    switch (tolower (str[0]))
    {
    case 'e':
        if (strncasecmp ("nc", str+1, 2) == 0)
        {
            const char * temp = NULL;

            if ((str[3] == '\0') || (str[3] == '&'))
                temp =  str + 3;

            if ((strncasecmp ("rypt", str+3, 4) == 0) && 
                ((str[7] == '\0') || (str[7] == '&')))
                temp = str + 7;
            if (temp)
            {
                KPOption * o;
                rc_t rc;

                rc = KPOptionMake (&o, kpopt_encrypted, temp, 0);
                if (rc)
                    return false;
                *opt = o;
                return temp;
            }
        }
        break;

    case 'p':
        if (strncasecmp ("wfile=", str + 1, sizeof ("wfile=") - 1) == 0)
        {
            const char * temp1 = str + 1 + sizeof ("wfile=") - 1;
            const char * temp2 = temp1;

            while ((*temp1 != '\0') && (*temp1 != '&'))
                ++temp1;
            if (temp1 != temp2)
            {
                KPOption * o;
                rc_t rc;

                rc = KPOptionMake (&o, kpopt_pwpath, temp2, temp1-temp2);
                if (rc)
                    return false;
                *opt = o;
                return temp1;
            }
            break;
        }
        if (strncasecmp ("wfd=", str + 1, sizeof ("wfd=") - 1) == 0)
        {
            const char * temp1 = str + 1 + sizeof ("wfd=") - 1;
            const char * temp2 = temp1;
            while (isdigit(*temp1))
                ++temp1;
            if (temp1 == temp2)
                break;
            if ((*temp1 == '\0') || (*temp1 == '&'))
            {
                KPOption * o;
                rc_t rc;

                rc = KPOptionMake (&o, kpopt_pwfd, temp2, temp1-temp2);
                if (rc)
                    return false;
                *opt = o;
                return temp1;
            }
        }
        break;

    }
    PATH_DEBUG (("%s: failed '%s'\n",__func__,str));
    return NULL;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_kfs_query (const char * str, BSTree * tree)
{
    /* 
     *    query       = query_entry [ * ( "&" query_entry ) ]
     *
     *    query_entry = "encrypt" / "enc" / ( "pwfile=" hier-part ) / ( "pwfd=" fd )
     */
    const char * temp;

    if (*str == '\0')
        return true;

    for (;;)
    {
        KPOption * o = NULL;

        temp = eat_kfs_query (str, &o);
        if ((temp == NULL) || (o == NULL))
            return false;

        /* can only have one of these two */
        if ((o->name == kpopt_pwpath) || (o->name == kpopt_pwfd))
        {
            BSTNode * n;

            n = BSTreeFind ( tree, (void*)kpopt_pwpath, KPOptionCmp );
            if (n)
                return false;

            n = BSTreeFind ( tree, (void*)kpopt_pwfd, KPOptionCmp );
            if (n)
                return false;

            if ( BSTreeInsert ( tree, &o->node, KPOptionSort ) != 0)
                return false;
        }
        else if ( o->name == kpopt_encrypted )
        {
            BSTNode * n;

            n = BSTreeFind ( tree, (void*)kpopt_pwpath, KPOptionCmp );
            if (n == NULL)
                if ( BSTreeInsert ( tree, &o->node, KPOptionSort ) != 0)
                    return false;
        }
        str = temp;
        if (*str == '\0')
            break;
        if (*str == '&')
            /**(char *)str = '\0'*/;
        else
            return false;
        ++str;
    }
    return true;
}
#endif
#if USE_EXPERIMENTAL_CODE
static
bool is_kfs_fragment (const char * str)
{
    return (*str == '\0');
}
#endif


#if USE_EXPERIMENTAL_CODE
#if SUPPORT_FILE_URL
/*
 * file://host/path bue we allow file:/path
 *
 * RFC-1738
 *
 * fileurl = "file://" [host / "localhost" ] "/" fpath
 */
static
rc_t KPathMakeUriFile (KPath * self, char * new_allocation, 
                       size_t sz, char * hier, char * query,
                       char * fragment)
{
    rc_t rc;
    assert (self);
    assert (new_allocation);
    assert (sz);
    assert (hier);
    assert (query);
    assert (fragment);

    if ((!is_file_hier (hier)) ||
        (!is_file_query (query)) ||
        (!is_file_fragment (fragment)))
    {
        return RC (rcFS, rcPath, rcConstructing, rcUri, rcInvalid);
    }

    free (self->storage);
    self->storage = new_allocation;
    self->alloc_size = sz;

    PATH_DEBUG (("%s: hier '%s' query '%s' fragment '%s'\n", __func__,
                 hier, query, fragment));
    rc = KPathTransformPathHier (&hier);
    PATH_DEBUG (("%s: hier '%s' query '%s' fragment '%s'\n", __func__,
                 hier, query, fragment));
    if (rc)
        return rc;
    StringInitCString (&self->path, hier);
    self->query = query;
    self->fragment = fragment;
    PATH_DEBUG (("%s: path '%S' fragment '%s'\n", __func__,
                 &self->path, self->fragment));
    return 0;
}        
#endif
#endif
#if USE_EXPERIMENTAL_CODE
#if SUPPORT_FILE_URL
static
rc_t KPathMakeUriKfs (KPath * self, char * new_allocation, 
                      size_t sz, char * hier,
                      char * query, char * fragment)
{
    rc_t rc;
    assert (self);
    assert (new_allocation);
    assert (sz);
    assert (hier);
    assert (query);
    assert (fragment);

    if (!is_kfs_hier (hier))
    {
        PATH_DEBUG (("%s: failed is_kfs_hier '%s'\n",__func__, hier));
        return RC (rcFS, rcPath, rcConstructing, rcUri, rcInvalid);
    }

    if (!is_kfs_query (query, &self->options))
    {
        PATH_DEBUG (("%s: failed is_kfs_query '%s'\n",__func__, query));
        return RC (rcFS, rcPath, rcConstructing, rcUri, rcInvalid);
    }

    if (!is_kfs_fragment (fragment))
    {
        PATH_DEBUG (("%s: failed is_kfs_fragment '%s'\n",__func__, fragment));
        return RC (rcFS, rcPath, rcConstructing, rcUri, rcInvalid);
    }

    free (self->storage);
    self->storage = new_allocation;
    self->alloc_size = sz;

    PATH_DEBUG (("%s: hier '%s' query '%s' fragment '%s'\n", __func__,
                 hier, query, fragment));
    rc = KPathTransformPathHier (&hier);
    PATH_DEBUG (("%s: hier '%s' query '%s' fragment '%s'\n", __func__,
                 hier, query, fragment));
    if (rc)
        return rc;
    StringInitCString (&self->path, hier);
    self->fragment = fragment;
    PATH_DEBUG (("%s: path '%S' fragment '%s'\n", __func__,
                 &self->path, self->fragment));
    return 0;
}
#endif
#endif
#if USE_EXPERIMENTAL_CODE
static
KPUri_t scheme_type (const char * scheme)
{
    /* We have a "legal" scheme name. We'll only look for specific schemes we
     * support and mark all others as merely unsupported rather than 
     * differentiate types we don't care about.
     */
#if SUPPORT_FILE_URL
    if (strcasecmp ("file", scheme) == 0)
    {
        PATH_DEBUG (("%s: file scheme\n",__func__));
        return kpuri_file;
    }
#endif

    if (strcasecmp (NCBI_FILE_SCHEME, scheme) == 0)
    {
        PATH_DEBUG (("%s: " NCBI_FILE_SCHEME " scheme\n",__func__));
        return kpuri_ncbi_kfs;
    }

    return kpuri_not_supported;
}
#endif


/*
 * See RFC 3986 / RFC 3987
 * We will allow utf-8 in our URI with the extended Unicode characters not 
 * required to be %-encoded.  We would have to do this encoding if we wish
 * to pass this uri out of our environment.
 *
 * we demand a valid set of characters for the scheme but do no validation 
 * of the other parts waiting for a scheme specific parsing
 */

#if USE_EXPERIMENTAL_CODE
/* pcopy and scheme point to the same place - seems redundant */
static
rc_t KPathSplitUri (KPath * self, char ** pcopy, size_t * psiz, char ** scheme, char ** hier,
                    char ** query, char ** fragment)
{
    char * copy;
    size_t z;

    assert (self && pcopy && psiz && scheme && hier && query && fragment);

    *pcopy = *scheme = *hier = *query = *fragment = NULL;
    *psiz = 0;

    z = self->asciz_size + 1;
    copy = malloc (z);
    if (copy == NULL)
        return RC (rcFS, rcPath, rcConstructing, rcMemory, rcExhausted);
    strcpy (copy, self->storage);
    for (;;)
    {
        char * s; /* start/scheme */
        char * h; /* hier-part */
        char * q; /* query */
        char * f; /* fragment */
        char * e; /* EOS (terminating NUL */

        s = copy;

        /* point at NUL at end */
        f = q = e = s + strlen (self->storage);

        /* find the scheme - terminated by first ':' from the beginning (left) */
        h = strchr (s, ':');

        /* scheme must be present as must the ':' and not % encoded
         * and must have some size
         * the character set for scheme is very limited - ASCII Alphanumeric
         * with "-", ",", "_", and "~"
         */
        if ((h == NULL) || (h == s))
            break;

        *h++ = '\0';
        if (! is_scheme (s))
            break;
        
        if (h != e)
        {
            f = strchr (h, '#');

            if (f == NULL)
                f = e;
            else
                *f++ = '\0';

            q = strchr (h, '?');

            if (q == NULL)
                q = e;
            else
                *q++ = '\0';
        }

        if (! string_decode (h))
            break;

        if (! string_decode (q))
            break;

        if (! string_decode (f))
            break;
        
        if (! is_hier (h))
            break;

        if (! is_query (q))
            break;

        if (! is_fragment (f))
            break;

        *scheme = s;
        *hier = h;
        *query = q;
        *fragment = f;
        *psiz = z;      /* WHAT???? */
        *pcopy = copy;
        return 0;
    }

    free (copy);
    return RC (rcFS, rcPath, rcParsing, rcUri, rcInvalid);
}
#endif


#if USE_EXPERIMENTAL_CODE
static
rc_t KPathParseURI (KPath * self)
{
    char * parsed_uri;
    char * scheme;
    char * hier;
    char * query;
    char * fragment;
    size_t allocated;
    rc_t rc;

    rc = KPathSplitUri (self, &parsed_uri, &allocated, &scheme, &hier, &query, &fragment);
    PATH_DEBUG (("%s: allocated %p '%zu'\n",__func__,parsed_uri,allocated));
    if (rc == 0)
    {
        switch (scheme_type (scheme))
        {
        case kpuri_invalid:
            rc = RC (rcFS, rcPath, rcParsing, rcUri, rcInvalid);
            break;

        case kpuri_not_supported:
            rc = RC (rcFS, rcPath, rcParsing, rcUri, rcIncorrect);
            break;

#if SUPPORT_FILE_URL
        case kpuri_file:
            PATH_DEBUG (("%s: call KPathMakeUriFile\n",__func__));
            rc = KPathMakeUriFile (self, parsed_uri, allocated, hier, query,
                                   fragment);
            break;
#endif

        case kpuri_ncbi_kfs:
            PATH_DEBUG (("%s: call KPathMakeUriKfs\n",__func__));
            rc = KPathMakeUriKfs (self, parsed_uri, allocated, hier, query,
                                  fragment);
            break;

        default:
            rc = RC (rcFS, rcPath, rcParsing, rcUri, rcCorrupt);
            break;
        }
        if (rc)
            free (parsed_uri);
    }
    return rc;
}
#endif

static
rc_t KPathAlloc (KPath ** pself, const char * path_string)
{
    rc_t rc;
    size_t z, zz;
    KPath * self;


    zz = 1 + (z = string_size (path_string));

    OFF_PATH_DEBUG(("%s: %s 'z' '%zu' zz '%zu'\n",__func__,path_string,z,zz));

    self = calloc (sizeof (*self), 1);
    if (self == NULL)
        rc = RC (rcFS, rcPath, rcConstructing, rcMemory, rcExhausted);
    else
    {
        self->storage = malloc (zz);
        if (self->storage == NULL)
            rc = RC (rcFS, rcPath, rcConstructing, rcMemory, rcExhausted);
        else
        {

            PATH_DEBUG (("-----\n%s: %p %zu %p %zu\n\n", __func__, self, sizeof (*self),
                         self->storage, zz));
            self->alloc_size = zz;

            memcpy (self->storage, path_string, zz);

            KRefcountInit (&self->refcount, 1, class_name, "init", self->storage);

            self->asciz_size = z;

            StringInit (&self->path, self->storage, z, string_len(self->storage, z));

            BSTreeInit (&self->options);

            self->fragment = self->storage + z;

            *pself = self;

            OFF_PATH_DEBUG (("%s: path '%S'\n", __func__, &self->path));

            return 0;
        }

        free (self);
    }
    return rc;
}


static
rc_t KPathMakeValidateParams (KPath ** new_path, const char * path)
{
    if (new_path == NULL)
        return RC (rcFS, rcPath, rcConstructing, rcSelf, rcNull);
    *new_path = NULL;
    if (path == NULL)
        return RC (rcFS, rcPath, rcConstructing, rcParam, rcNull);
    return 0;
}


LIB_EXPORT rc_t CC KPathMake ( KPath ** new_path, const char * posix_path)
{
    KPath * self;
    rc_t rc;

    rc = KPathMakeValidateParams (new_path, posix_path);
    if (rc == 0)
    {
        rc = KPathAlloc (&self, posix_path);
        if (rc == 0)
        {
#if USE_EXPERIMENTAL_CODE
            rc = KPathParseURI (self);
#endif

            /* ignore return - if its bad just leave the path alone even if 
             * it turns out to be bad later */
            *new_path = self;
            return 0;
        }
    }
    return rc;
}


LIB_EXPORT rc_t CC KPathMakeSysPath ( KPath ** new_path, const char * sys_path)
{
    KPath * self;
    rc_t rc;

    rc = KPathMakeValidateParams (new_path, sys_path);
    if (rc)
    {
        LOGERR (klogErr, rc, "error with KPathMakeValidateParams");
        return rc;
    }

    rc = KPathAlloc (&self, sys_path);
    if (rc)
        return rc;
#if USE_EXPERIMENTAL_CODE
    /* first try as URI then as a system specific path */
    rc = KPathParseURI (self);
    if (rc)
#endif
        rc = KPathTransformSysPath(self);

    if (rc)
        KPathDestroy (self);
    else
        *new_path = self;
    return rc;
}


LIB_EXPORT rc_t CC KPathMakeVFmt ( KPath ** new_path, const char * fmt, va_list args )
{
    size_t len;
    rc_t rc;
    char buffer [32*1024]; /* okay we really don't want any larger than this I suppose */

    rc = KPathMakeValidateParams (new_path, fmt);
    if (rc)
        return rc;

    rc = string_printf (buffer, sizeof (buffer), &len, fmt, args);
    if (rc)
        return rc;

    if (len >= sizeof buffer)
        return RC (rcFS, rcPath, rcConstructing, rcBuffer, rcInsufficient);

    return KPathMake (new_path, buffer);
}


LIB_EXPORT rc_t CC KPathMakeFmt ( KPath ** new_path, const char * fmt, ... )
{
    rc_t rc;
    va_list args;

    va_start (args, fmt);
    rc = KPathMakeVFmt (new_path, fmt, args);
    va_end (args);

    return rc;
}

LIB_EXPORT rc_t CC KPathMakeRelative ( KPath ** new_path, const KPath * base_path,
                                       const char * relative_path )
{
    rc_t rc = RC (rcFS, rcPath, rcConstructing, rcFunction, rcUnsupported);



    return rc;
}


LIB_EXPORT rc_t CC KPathMakeCurrentPath ( KPath ** new_path )
{
    char buff [4096];
    char * a = NULL;
    char * b = buff;
    size_t z;
    rc_t rc;

    rc = KPathGetCWD (b, sizeof buff);
    if (rc)
    {
        for (z = 2 * sizeof buff; rc; z += sizeof buff)
        {
            b = realloc (a, z);
            if (b == NULL)
            {
                free (a);
                return RC (rcFS, rcPath, rcConstructing, rcMemory, rcExhausted);
            }
            rc = KPathGetCWD (b, z);
        }
    }
    rc = KPathMakeSysPath (new_path, b);
    if (b != buff)
        free (b);
    return rc;
}

/* ----------
 * KPathReadPath
 *
 * Copy the path as a ASCIZ string to the buffer.  The form will be the KFS
 * internal "posix-path" form.
 */
LIB_EXPORT rc_t CC KPathReadPath (const KPath * self, char * buffer, size_t buffer_size,
                                  size_t * num_read)
{
    size_t z = StringSize (&self->path);

    if (buffer_size < z)
        return RC (rcFS, rcPath, rcReading, rcBuffer, rcInsufficient);


    PATH_DEBUG (("%s: path '%S' fragment '%s'\n", __func__,
                 &self->path, self->fragment));

    PATH_DEBUG (("%s: should copy '%*.*s' length '%zu'\n", __func__, z, z,
                 self->path.addr, z));
    memcpy (buffer, self->path.addr, z);
    if (buffer_size > z)
        buffer[z] = '\0';
    *num_read = z;

    PATH_DEBUG (("%s: copied '%*.*s' length '%zu'\n", __func__, z, z,
                 buffer, z));
    return 0;
}

LIB_EXPORT rc_t CC KPathReadQuery (const KPath * self, char * buffer, size_t buffer_size,
                                   size_t * num_read)
{
    if (num_read == NULL)
        return RC (rcFS, rcPath, rcAccessing, rcParam, rcNull);
    *num_read = 0;

    if (buffer == NULL)
        return RC (rcFS, rcPath, rcAccessing, rcParam, rcNull);

    if (self == NULL)
        return RC (rcFS, rcPath, rcAccessing, rcSelf, rcNull);

    *num_read = string_copy (buffer, buffer_size, self->query,
                             self->alloc_size);
    return 0;
}

LIB_EXPORT rc_t CC KPathReadFragment (const KPath * self, char * buffer, size_t buffer_size,
                                      size_t * num_read)
{
    if (num_read == NULL)
        return RC (rcFS, rcPath, rcAccessing, rcParam, rcNull);
    *num_read = 0;

    if (buffer == NULL)
        return RC (rcFS, rcPath, rcAccessing, rcParam, rcNull);

    if (self == NULL)
        return RC (rcFS, rcPath, rcAccessing, rcSelf, rcNull);



    *num_read = string_copy (buffer, buffer_size, self->fragment, 
                             self->alloc_size);
    return 0;
}

/* ----------
 */
LIB_EXPORT rc_t CC KPathOption (const KPath * self, KPOption_t option,
                                char * buffer, size_t buffer_size,
                                size_t * num_read)
{
    size_t o = (size_t)option;
    BSTNode * n = BSTreeFind (&self->options, (void*)o, KPOptionCmp);
    KPOption * opt;

    if (n == NULL)
        return RC (rcFS, rcPath, rcAccessing, rcParam, rcNotFound);
    opt = (KPOption*)n;
    *num_read = string_copy (buffer, buffer_size, opt->value.addr, opt->value.size);
    return 0;
}
