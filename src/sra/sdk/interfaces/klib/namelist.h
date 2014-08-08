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

#ifndef _h_klib_namelist_
#define _h_klib_namelist_

#ifndef _h_klib_extern_
#include <klib/extern.h>
#endif

#ifndef _h_klib_defs_
#include <klib/defs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * KNamelist
 *  a generic list of NUL-terminated name strings
 */
typedef struct KNamelist KNamelist;

/* AddRef
 * Release
 *  ignores NULL references
 */
KLIB_EXTERN rc_t CC KNamelistAddRef ( const KNamelist *self );
KLIB_EXTERN rc_t CC KNamelistRelease ( const KNamelist *self );

/* Count
 *  returns the number of entries
 *
 *  "count" [ OUT ] - return parameter for entry count
 */
KLIB_EXTERN rc_t CC KNamelistCount ( const KNamelist *self, uint32_t *count );

/* Get
 *  get an indexed name
 *
 *  "idx" [ IN ] - a zero-based name index
 *
 *  "name" [ OUT ] - return parameter for NUL terminated name
 */
KLIB_EXTERN rc_t CC KNamelistGet ( const KNamelist *self,
    uint32_t idx, const char **name );


/*--------------------------------------------------------------------------
 * VNamelist
 *  generic Vector namelist implementation
 */
typedef struct VNamelist VNamelist;

/* Make
 *  make an empty namelist
 *
 *  "names" [ OUT ] - return parameter for namelist object
 *
 *  "alloc_blocksize" [ IN ] - selects the number of names in
 *  a vector block; used for allocating and extending
 */
KLIB_EXTERN rc_t CC VNamelistMake ( VNamelist **names, const uint32_t alloc_blocksize );

/* Release
 *  ignores NULL references
 */
KLIB_EXTERN rc_t CC VNamelistRelease ( const VNamelist *self );

/* ToNamelist
 *  cast operator
 *
 *  "cast" [ OUT ] - return parameter for new KNamelist reference
 *  must be released by KNamelistRelease
 */
KLIB_EXTERN rc_t CC VNamelistToNamelist ( VNamelist *self, KNamelist **cast );
KLIB_EXTERN rc_t CC VNamelistToConstNamelist ( const VNamelist *self, const KNamelist **cast );

/* AppendCopy
 *  appends a copy of string to namelist
 *
 *  "src" [ IN ] - NUL terminated name string to be copied
 *  and appended.
 */
KLIB_EXTERN rc_t CC VNamelistAppend ( VNamelist *self, const char* src );

/* Remove
 *  removes a string from the namelist
 *
 *  "s" [ IN ] - NUL terminated name string to be removed
 */
KLIB_EXTERN rc_t CC VNamelistRemove( VNamelist *self, const char* s );

/* IndexOf
 *  searches linear in the namelist for the string
 *
 *  "s" [ IN ] - NUL terminated name string to be searched for
 *
 *  "found" [ OUT ] - index of the string if found
 *  unchanged if not found
 */
KLIB_EXTERN rc_t CC VNamelistIndexOf( VNamelist *self, const char* s, uint32_t *found );

/* Count
 *  returns the number of entries
 *
 *  "count" [ OUT ] - return parameter for entry count
 */
KLIB_EXTERN rc_t CC VNameListCount ( const VNamelist *self, uint32_t *count );

/* Get
 *  get an indexed name
 *
 *  "idx" [ IN ] - a zero-based name index
 *
 *  "name" [ OUT ] - return parameter for NUL terminated name
 */
KLIB_EXTERN rc_t CC VNameListGet ( const VNamelist *self, uint32_t idx, const char **name );


/* Reorder
 *  sort the names according to case sensitivity
 *  and UNICODE character code ordering
 *
 *  "case_insensitive" [ IN ] - when true, perform "tolower" on
 *   each character before compare
 */
KLIB_EXTERN void CC VNamelistReorder ( VNamelist *self, bool case_insensitive );

#ifdef __cplusplus
}
#endif

#endif /* _h_klib_namelist_ */
