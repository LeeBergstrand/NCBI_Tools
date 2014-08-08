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

#include <kfg/extern.h>

#include <kfg/repository.h>
#include <kfg/config.h>
#include <kfs/file.h>
#include <kfs/directory.h>
#include <kfs/impl.h>
#include <klib/refcount.h>
#include <klib/text.h>
#include <klib/vector.h>
#include <klib/namelist.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>


/*--------------------------------------------------------------------------
 * KRepository
 *  presents structured access to a storage repository
 *  as modeled in KConfig.
 *
 *  all objects are obtained via KRepositoryMgr ( see below )
 */
struct KRepository
{
    const KConfigNode *node;
    String name;
    KRefcount refcount;
    KRepCategory category;
    KRepSubCategory subcategory;
};


/* Whack
 */
static
rc_t KRepositoryWhack ( KRepository *self )
{
    KConfigNodeRelease ( self -> node );
    free ( self );
    return 0;
}

/* Sort
 */
static
int CC KRepositorySort ( const void **a, const void **b, void *ignore )
{
    const KRepository *left = * a;
    const KRepository *right = * b;

    if ( left -> category < right -> category )
        return -1;
    if ( left -> category > right -> category )
        return 1;
    if ( left -> subcategory < right -> subcategory )
        return -1;
    if ( left -> subcategory > right -> subcategory )
        return 1;
    return StringCompare ( & left -> name, & right -> name );
}

/* Make
 */
static
rc_t KRepositoryMake ( KRepository **rp, const KConfigNode *node,
    const char *name, KRepCategory category, KRepSubCategory subcategory )
{
    rc_t rc;
    KRepository *r;
    String name_str;

    /* measure string */
    StringInitCString ( & name_str, name );

    /* create object */
    r = malloc ( sizeof * r + name_str . size + 1 );
    if ( r == NULL )
        return RC ( rcKFG, rcNode, rcConstructing, rcMemory, rcExhausted );

    rc = KConfigNodeAddRef ( node );
    if ( rc != 0 )
    {
        free ( r );
        return rc;
    }

    r -> node = node;
    r -> name = name_str;
    r -> name . addr = ( char* ) ( r + 1 );
    KRefcountInit ( & r -> refcount, 1, "KRepository", "make", name );
    r -> category = category;
    r -> subcategory = subcategory;
    memcpy ( r + 1, name, name_str . size );
    ( ( char* ) ( r + 1 ) ) [ name_str . size ] = 0;

    * rp = r;

    return 0;
}


/* AddRef
 * Release
 */
LIB_EXPORT rc_t CC KRepositoryAddRef ( const KRepository *self )
{
    if ( self != NULL )
    {
        switch ( KRefcountAdd ( & self -> refcount, "KRepository" ) )
        {
        case krefLimit:
            return RC ( rcKFG, rcNode, rcAttaching, rcRange, rcExcessive );
        }
    }
    return 0;
}

LIB_EXPORT rc_t CC KRepositoryRelease ( const KRepository *self )
{
    if ( self != NULL )
    {
        switch ( KRefcountDrop ( & self -> refcount, "KRepository" ) )
        {
        case krefWhack:
            return KRepositoryWhack ( ( KRepository* ) self );
        case krefLimit:
            return RC ( rcKFG, rcNode, rcReleasing, rcRange, rcExcessive );
        }
    }
    return 0;
}


/* WhackEntry
 *  for cleaning up these vectors
 */
static
void CC KRepositoryWhackEntry ( void *item, void *ignore )
{
    KRepository *self = item;
    KRepositoryRelease ( self );
}


/* Category
 * SubCategory
 *  tells what the repository category or sub-category are
 *  or returns "bad" if the repository object is not usable.
 */
LIB_EXPORT KRepCategory CC KRepositoryCategory ( const KRepository *self )
{
    if ( self != NULL )
        return self -> category;
    return krepBadCategory;
}

LIB_EXPORT KRepSubCategory CC KRepositorySubCategory ( const KRepository *self )
{
    if ( self != NULL )
        return self -> subcategory;
    return krepBadSubCategory;
}


/* Name
 *  get the repository name
 *  attempts to copy NUL-terminated name into provided buffer
 *
 *  "buffer" [ OUT ] and "bsize" [ IN ] - name output parameter
 *
 *  "name_size" [ OUT, NULL OKAY ] - returns the name size in
 *  bytes, excluding any NUL termination.
 */
LIB_EXPORT rc_t CC KRepositoryName ( const KRepository *self,
    char *buffer, size_t bsize, size_t *name_size )
{
    if ( self == NULL )
        return RC ( rcKFG, rcNode, rcAccessing, rcSelf, rcNull );

    if ( name_size != NULL )
        * name_size = self -> name . size;

    if ( bsize < self -> name . size )
        return RC ( rcKFG, rcNode, rcAccessing, rcBuffer, rcInsufficient );

    if ( buffer == NULL )
        return RC ( rcKFG, rcNode, rcAccessing, rcBuffer, rcNull );

    memcpy ( buffer, self -> name . addr, self -> name . size );

    if ( bsize > self -> name . size )
        buffer [ self -> name . size ] = 0;

    return 0;
}


/* DisplayName
 *  get the repository display name,
 *  if different from its actual name
 *
 *  attempts to copy NUL-terminated name into provided buffer
 *
 *  "buffer" [ OUT ] and "bsize" [ IN ] - name output parameter
 *
 *  "name_size" [ OUT, NULL OKAY ] - returns the name size in
 *  bytes, excluding any NUL termination.
 */
LIB_EXPORT rc_t CC KRepositoryDisplayName ( const KRepository *self,
    char *buffer, size_t bsize, size_t *name_size )
{
    rc_t rc;

    if ( self == NULL )
        rc = RC ( rcKFG, rcNode, rcAccessing, rcSelf, rcNull );
    else
    {
        const KConfigNode *node;

        if ( name_size != NULL )
            * name_size = 0;

        rc = KConfigNodeOpenNodeRead ( self -> node, & node, "display-name" );
        if ( rc != 0 )
            rc = KRepositoryName ( self, buffer, bsize, name_size );
        else
        {
            size_t num_read, remaining;
            rc = KConfigNodeRead ( node, 0, buffer, bsize, & num_read, & remaining );
            KConfigNodeRelease ( node );

            if ( rc == 0 )
            {
                if ( name_size != NULL )
                    * name_size = num_read + remaining;

                if ( remaining != 0 )
                    rc = RC ( rcKFG, rcNode, rcAccessing, rcBuffer, rcInsufficient );
                else if ( num_read < bsize )
                    buffer [ num_read ] = 0;
            }
        }
    }

    return rc;
}


/* Root
 *  read the root path as a POSIX path or URL
 *
 *  attempts to copy NUL-terminated path into provided buffer
 *
 *  "buffer" [ OUT ] and "bsize" [ IN ] - path output parameter
 *
 *  "root_size" [ OUT, NULL OKAY ] - returns the path size in
 *  bytes, excluding any NUL termination.
 */
LIB_EXPORT rc_t CC KRepositoryRoot ( const KRepository *self,
    char *buffer, size_t bsize, size_t *root_size )
{
    rc_t rc;

    if ( self == NULL )
        rc = RC ( rcKFG, rcNode, rcAccessing, rcSelf, rcNull );
    else
    {
        const KConfigNode *node;

        if ( root_size != NULL )
            * root_size = 0;

        rc = KConfigNodeOpenNodeRead ( self -> node, & node, "root" );
        if ( rc == 0 )
        {
            size_t num_read, remaining;
            rc = KConfigNodeRead ( node, 0, buffer, bsize, & num_read, & remaining );
            KConfigNodeRelease ( node );

            if ( rc == 0 )
            {
                if ( root_size != NULL )
                    * root_size = num_read + remaining;

                if ( remaining != 0 )
                    rc = RC ( rcKFG, rcNode, rcAccessing, rcBuffer, rcInsufficient );
                else if ( num_read < bsize )
                    buffer [ num_read ] = 0;
            }
        }
    }

    return rc;
}


/* Disabled
 *  discover whether the repository is enabled
 */
LIB_EXPORT bool CC KRepositoryDisabled ( const KRepository *self )
{
    if ( self != NULL )
    {
        const KConfigNode *node;
        rc_t rc = KConfigNodeOpenNodeRead ( self -> node, & node, "disabled" );
        if ( rc == 0 )
        {
            bool disabled = false;
            rc = KConfigNodeReadBool ( node, & disabled );
            KConfigNodeRelease ( node );
            if ( rc == 0 )
                return disabled;
        }
    }

    return false;
}

/* CacheEnabled
 *  discover whether the repository supports caching
 */
LIB_EXPORT bool CC KRepositoryCacheEnabled ( const KRepository *self )
{
    if ( self != NULL )
    {
        const KConfigNode *node;
        rc_t rc = KConfigNodeOpenNodeRead ( self -> node, & node, "cache-enabled" );
        if ( rc == 0 )
        {
            bool enabled = false;
            rc = KConfigNodeReadBool ( node, & enabled );
            KConfigNodeRelease ( node );
            if ( rc == 0 )
                return enabled;
        }
    }

    return false;
}


/* DownloadTicket
 *  return any associated download ticket
 *
 *  attempts to copy NUL-terminated ticket into provided buffer
 *
 *  "buffer" [ OUT ] and "bsize" [ IN ] - ticket output parameter
 *
 *  "ticket_size" [ OUT, NULL OKAY ] - returns the ticket size in
 *  bytes, excluding any NUL termination.
 */
LIB_EXPORT rc_t CC KRepositoryDownloadTicket ( const KRepository *self,
    char *buffer, size_t bsize, size_t *ticket_size )
{
    rc_t rc;

    if ( self == NULL )
        rc = RC ( rcKFG, rcNode, rcAccessing, rcSelf, rcNull );
    else
    {
        const KConfigNode *node;

        if ( ticket_size != NULL )
            * ticket_size = 0;

        rc = KConfigNodeOpenNodeRead ( self -> node, & node, "download-ticket" );
        if ( rc == 0 )
        {
            size_t num_read, remaining;
            rc = KConfigNodeRead ( node, 0, buffer, bsize, & num_read, & remaining );
            KConfigNodeRelease ( node );

            if ( rc == 0 )
            {
                if ( ticket_size != NULL )
                    * ticket_size = num_read + remaining;

                if ( remaining != 0 )
                    rc = RC ( rcKFG, rcNode, rcAccessing, rcBuffer, rcInsufficient );
                else if ( num_read < bsize )
                    buffer [ num_read ] = 0;
            }
        }
    }

    return rc;
}


/* EncryptionKey
 *  return any associated encryption key
 *
 *  attempts to copy NUL-terminated key into provided buffer
 *
 *  "buffer" [ OUT ] and "bsize" [ IN ] - encryption key output parameter
 *
 *  "key_size" [ OUT, NULL OKAY ] - returns the key size in
 *  bytes, excluding any NUL termination.
 */
LIB_EXPORT rc_t CC KRepositoryEncryptionKey ( const KRepository *self,
    char *buffer, size_t bsize, size_t *key_size )
{
    rc_t rc;

    if ( self == NULL )
        rc = RC ( rcKFG, rcNode, rcAccessing, rcSelf, rcNull );
    else
    {
        const KConfigNode *node;

        if ( key_size != NULL )
            * key_size = 0;

        rc = KConfigNodeOpenNodeRead ( self -> node, & node, "encryption-key" );
        if ( rc == 0 )
        {
            size_t num_read, remaining;
            rc = KConfigNodeRead ( node, 0, buffer, bsize, & num_read, & remaining );
            KConfigNodeRelease ( node );

            if ( rc == 0 )
            {
                if ( key_size != NULL )
                    * key_size = num_read + remaining;

                if ( remaining != 0 )
                    rc = RC ( rcKFG, rcNode, rcAccessing, rcBuffer, rcInsufficient );
                else if ( num_read < bsize )
                    memset ( & buffer [ num_read ], 0, bsize - num_read );
            }
        }
        else if ( GetRCState ( rc ) == rcNotFound )
        {
            char path [ 4096 ];
            rc_t rc2 = KRepositoryEncryptionKeyFile ( self, path, sizeof path, NULL );
            if ( rc2 == 0 )
            {
                KDirectory *wd;
                rc2 = KDirectoryNativeDir ( & wd );
                if ( rc2 == 0 )
                {
                    const KFile *keyFile;
                    rc2 = KDirectoryOpenFileRead ( wd, & keyFile, path );
                    KDirectoryRelease ( wd );
                    if ( rc2 == 0 )
                    {
                        size_t num_read;
                        rc = KFileReadAll ( keyFile, 0, buffer, bsize, & num_read );
                        if ( rc == 0 )
                        {
                            if ( num_read == bsize )
                            {
                                uint64_t eof;
                                rc = KFileSize ( keyFile, & eof );
                                if ( rc == 0 )
                                    num_read = ( size_t ) eof;
                                else
                                    num_read = 0;

                                rc = RC ( rcKFG, rcFile, rcReading, rcBuffer, rcInsufficient );
                                memset ( buffer, 0, bsize );
                            }
                            else if ( num_read == 0 )
                            {
                                rc = RC ( rcKFG, rcFile, rcReading, rcFile, rcEmpty );
                                memset ( buffer, 0, bsize );
                            }
                            else
                            {
                                char *eoln = string_chr ( buffer, num_read, '\n' );
                                if ( eoln != NULL )
                                {
                                    if ( eoln == buffer )
                                        num_read = 0;
                                    else if ( eoln [ -1 ] == '\r' )
                                        num_read = eoln - buffer - 1;
                                    else
                                        num_read = eoln - buffer;
                                }

                                if ( key_size != NULL )
                                    * key_size = num_read;

                                memset ( & buffer [ num_read ], 0, bsize - num_read );
                            }
                        }

                        KFileRelease ( keyFile );
                    }
                }
            }
        }
    }

    return rc;
}


/* EncryptionKeyFile
 *  return path to any associated encryption key file
 *
 *  attempts to copy NUL-terminated path into provided buffer
 *
 *  "buffer" [ OUT ] and "bsize" [ IN ] - key file path output parameter
 *
 *  "path_size" [ OUT, NULL OKAY ] - returns the path size in
 *  bytes, excluding any NUL termination.
 */
LIB_EXPORT rc_t CC KRepositoryEncryptionKeyFile ( const KRepository *self,
    char *buffer, size_t bsize, size_t *path_size )
{
    rc_t rc;

    if ( self == NULL )
        rc = RC ( rcKFG, rcNode, rcAccessing, rcSelf, rcNull );
    else
    {
        const KConfigNode *node;

        if ( path_size != NULL )
            * path_size = 0;

        rc = KConfigNodeOpenNodeRead ( self -> node, & node, "encryption-key-path" );
        if ( rc == 0 )
        {
            size_t num_read, remaining;
            rc = KConfigNodeRead ( node, 0, buffer, bsize, & num_read, & remaining );
            KConfigNodeRelease ( node );

            if ( rc == 0 )
            {
                if ( path_size != NULL )
                    * path_size = num_read + remaining;

                if ( remaining != 0 )
                    rc = RC ( rcKFG, rcNode, rcAccessing, rcBuffer, rcInsufficient );
                else if ( num_read < bsize )
                    buffer [ num_read ] = 0;
            }
        }
    }

    return rc;
}


/* Description
 *  return any associated descriptive text
 *
 *  attempts to copy NUL-terminated description into provided buffer
 *
 *  "buffer" [ OUT ] and "bsize" [ IN ] - description text output parameter
 *
 *  "desc_size" [ OUT, NULL OKAY ] - returns the text size in
 *  bytes, excluding any NUL termination.
 */
LIB_EXPORT rc_t CC KRepositoryDescription ( const KRepository *self,
    char *buffer, size_t bsize, size_t *desc_size )
{
    rc_t rc;

    if ( self == NULL )
        rc = RC ( rcKFG, rcNode, rcAccessing, rcSelf, rcNull );
    else
    {
        const KConfigNode *node;

        if ( desc_size != NULL )
            * desc_size = 0;

        rc = KConfigNodeOpenNodeRead ( self -> node, & node, "description" );
        if ( rc == 0 )
        {
            size_t num_read, remaining;
            rc = KConfigNodeRead ( node, 0, buffer, bsize, & num_read, & remaining );
            KConfigNodeRelease ( node );

            if ( rc == 0 )
            {
                if ( desc_size != NULL )
                    * desc_size = num_read + remaining;

                if ( remaining != 0 )
                    rc = RC ( rcKFG, rcNode, rcAccessing, rcBuffer, rcInsufficient );
                else if ( num_read < bsize )
                    buffer [ num_read ] = 0;
            }
        }
    }

    return rc;
}


/*--------------------------------------------------------------------------
 * KRepositoryVector
 *  uses Vector API
 *  holds zero or more KRepository objects
 */


/* Whack
 *  destroy your vector
 */
LIB_EXPORT rc_t CC KRepositoryVectorWhack ( KRepositoryVector *self )
{
    if ( self == NULL )
        return RC ( rcKFG, rcVector, rcDestroying, rcSelf, rcNull );

    VectorWhack ( self, KRepositoryWhackEntry, NULL );
    return 0;
}


/*--------------------------------------------------------------------------
 * KRepositoryMgr
 *  manages structured access to repositories
 */
struct KRepositoryMgr
{
    const KConfig *ro_cfg;
    KConfig *rw_cfg;
    KRefcount refcount;
};


/* Whack
 */
static
rc_t KRepositoryMgrWhack ( KRepositoryMgr *self )
{
    if ( self -> ro_cfg )
        KConfigRelease ( self -> ro_cfg );
    if ( self -> rw_cfg )
        KConfigRelease ( self -> rw_cfg );
    free ( self );
    return 0;
}


/* Make
 *  create a repository manager
 *  uses values from "self"
 */
LIB_EXPORT rc_t CC KConfigMakeRepositoryMgrRead ( const KConfig *self, const KRepositoryMgr **mgrp )
{
    rc_t rc;

    if ( mgrp == NULL )
        rc = RC ( rcKFG, rcMgr, rcConstructing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcKFG, rcMgr, rcConstructing, rcSelf, rcNull );
        else
        {
            KRepositoryMgr *mgr = calloc ( 1, sizeof * mgr );
            if ( mgr == NULL )
                rc = RC ( rcKFG, rcMgr, rcConstructing, rcMemory, rcExhausted );
            else
            {
                rc = KConfigAddRef ( self );
                if ( rc == 0 )
                {
                    mgr -> ro_cfg = self;
                    KRefcountInit ( & mgr -> refcount, 1, "KRepositoryMgr", "make-read", "mgr" );
                    * mgrp = mgr;
                    return 0;
                }

                free ( mgr );
            }
        }

        * mgrp = NULL;
    }

    return rc;
}

LIB_EXPORT rc_t CC KConfigMakeRepositoryMgrUpdate ( KConfig *self, KRepositoryMgr **mgrp )
{
    rc_t rc;

    if ( mgrp == NULL )
        rc = RC ( rcKFG, rcMgr, rcConstructing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcKFG, rcMgr, rcConstructing, rcSelf, rcNull );
        else
        {
            KRepositoryMgr *mgr = calloc ( 1, sizeof * mgr );
            if ( mgr == NULL )
                rc = RC ( rcKFG, rcMgr, rcConstructing, rcMemory, rcExhausted );
            else
            {
                rc = KConfigAddRef ( self );
                if ( rc == 0 )
                {
                    mgr -> rw_cfg = self;
                    KRefcountInit ( & mgr -> refcount, 1, "KRepositoryMgr", "make-update", "mgr" );
                    * mgrp = mgr;
                    return 0;
                }

                free ( mgr );
            }
        }

        * mgrp = NULL;
    }

    return rc;
}


/* AddRef
 * Release
 */
LIB_EXPORT rc_t CC KRepositoryMgrAddRef ( const KRepositoryMgr *self )
{
    if ( self != NULL )
    {
        switch ( KRefcountAdd ( & self -> refcount, "KRepositoryMgr" ) )
        {
        case krefLimit:
            return RC ( rcKFG, rcMgr, rcAttaching, rcRange, rcExcessive );
        }
    }
    return 0;
}

LIB_EXPORT rc_t CC KRepositoryMgrRelease ( const KRepositoryMgr *self )
{
    if ( self != NULL )
    {
        switch ( KRefcountDrop ( & self -> refcount, "KRepositoryMgr" ) )
        {
        case krefWhack:
            return KRepositoryMgrWhack ( ( KRepositoryMgr* ) self );
        case krefLimit:
            return RC ( rcKFG, rcMgr, rcReleasing, rcRange, rcExcessive );
        }
    }
    return 0;
}


static
const KConfig *KRepositoryMgrGetROKConfig ( const KRepositoryMgr *self )
{
    return self -> ro_cfg ? self -> ro_cfg : self -> rw_cfg;
}

static
rc_t KRepositoryMgrSubCategoryRepositories ( const KConfigNode *sub,
    KRepCategory category, KRepSubCategory subcategory, KRepositoryVector *repositories )
{
    KNamelist *repo_names;
    rc_t rc = KConfigNodeListChildren ( sub, & repo_names );
    if ( rc == 0 )
    {
        uint32_t i, count;
        rc = KNamelistCount ( repo_names, & count );
        for ( i = 0; i < count && rc == 0; ++ i )
        {
            const char *repo_name;
            rc = KNamelistGet ( repo_names, i, & repo_name );
            if ( rc == 0 )
            {
                const KConfigNode *node;
                rc = KConfigNodeOpenNodeRead ( sub, & node, repo_name );
                if ( rc == 0 )
                {
                    KRepository *repo;
                    rc = KRepositoryMake ( & repo, node, repo_name, category, subcategory );
                    if ( rc == 0 )
                    {
                        rc = VectorAppend ( repositories, NULL, repo );
                        if ( rc != 0 )
                            KRepositoryWhack ( repo );
                    }

                    KConfigNodeRelease ( node );
                }
            }
        }

        KNamelistRelease ( repo_names );
    }

    return rc;
}

static
rc_t KRepositoryMgrCategoryRepositories ( const KConfigNode *cat,
    KRepCategory category, KRepositoryVector *repositories )
{
    KNamelist *sub_names;
    rc_t rc = KConfigNodeListChildren ( cat, & sub_names );
    if ( rc == 0 )
    {
        uint32_t i, count;
        rc = KNamelistCount ( sub_names, & count );
        for ( i = 0; i < count && rc == 0; ++ i )
        {
            const char *sub_name;
            rc = KNamelistGet ( sub_names, i, & sub_name );
            if ( rc == 0 )
            {
                KRepSubCategory subcategory = krepBadSubCategory;
                if ( strcmp ( "main", sub_name ) == 0 )
                    subcategory = krepMainSubCategory;
                else if ( strcmp ( "aux", sub_name ) == 0 )
                    subcategory = krepAuxSubCategory;
                else if ( strcmp ( "protected", sub_name ) == 0 )
                    subcategory = krepProtectedSubCategory;

                if ( subcategory != krepBadSubCategory )
                {
                    const KConfigNode *sub;
                    rc = KConfigNodeOpenNodeRead ( cat, & sub, sub_name );
                    if ( rc == 0 )
                    {
                        rc = KRepositoryMgrSubCategoryRepositories ( sub, category, subcategory, repositories );
                        KConfigNodeRelease ( sub );
                    }
                }
            }
        }

        KNamelistRelease ( sub_names );
    }

    return rc;
}


/* UserRepositories
 *  retrieve all user repositories in a Vector
 */
LIB_EXPORT rc_t CC KRepositoryMgrUserRepositories ( const KRepositoryMgr *self,
    KRepositoryVector *user_repositories )
{
    rc_t rc;

    if ( user_repositories == NULL )
        rc = RC ( rcKFG, rcMgr, rcAccessing, rcParam, rcNull );
    else
    {
        VectorInit ( user_repositories, 0, 8 );

        if ( self == NULL )
            rc = RC ( rcKFG, rcMgr, rcAccessing, rcSelf, rcNull );
        else
        {
            const KConfig *kfg = KRepositoryMgrGetROKConfig ( self );

            const KConfigNode *user;
            rc = KConfigOpenNodeRead ( kfg, & user, "/repository/user" );
            if ( rc == 0 )
            {
                rc = KRepositoryMgrCategoryRepositories ( user, krepUserCategory, user_repositories );
                KConfigNodeRelease ( user );
                if ( rc == 0 )
                    VectorReorder ( user_repositories, KRepositorySort, NULL );
            }

            if ( rc != 0 )
                KRepositoryVectorWhack ( user_repositories );
        }
    }

    return rc;
}


/* SiteRepositories
 *  retrieve all site repositories in a Vector
 */
LIB_EXPORT rc_t CC KRepositoryMgrSiteRepositories ( const KRepositoryMgr *self,
    KRepositoryVector *site_repositories )
{
    rc_t rc;

    if ( site_repositories == NULL )
        rc = RC ( rcKFG, rcMgr, rcAccessing, rcParam, rcNull );
    else
    {
        VectorInit ( site_repositories, 0, 8 );

        if ( self == NULL )
            rc = RC ( rcKFG, rcMgr, rcAccessing, rcSelf, rcNull );
        else
        {
            const KConfig *kfg = KRepositoryMgrGetROKConfig ( self );

            const KConfigNode *site;
            rc = KConfigOpenNodeRead ( kfg, & site, "/repository/site" );
            if ( rc == 0 )
            {
                rc = KRepositoryMgrCategoryRepositories ( site, krepSiteCategory, site_repositories );
                KConfigNodeRelease ( site );
                if ( rc == 0 )
                    VectorReorder ( site_repositories, KRepositorySort, NULL );
            }

            if ( rc != 0 )
                KRepositoryVectorWhack ( site_repositories );
        }
    }

    return rc;
}


/* RemoteRepositories
 *  retrieve all remote repositories in a Vector
 */
LIB_EXPORT rc_t CC KRepositoryMgrRemoteRepositories ( const KRepositoryMgr *self,
    KRepositoryVector *remote_repositories )
{
    rc_t rc;

    if ( remote_repositories == NULL )
        rc = RC ( rcKFG, rcMgr, rcAccessing, rcParam, rcNull );
    else
    {
        VectorInit ( remote_repositories, 0, 8 );

        if ( self == NULL )
            rc = RC ( rcKFG, rcMgr, rcAccessing, rcSelf, rcNull );
        else
        {
            const KConfig *kfg = KRepositoryMgrGetROKConfig ( self );

            const KConfigNode *remote;
            rc = KConfigOpenNodeRead ( kfg, & remote, "/repository/remote" );
            if ( rc == 0 )
            {
                rc = KRepositoryMgrCategoryRepositories ( remote, krepRemoteCategory, remote_repositories );
                KConfigNodeRelease ( remote );
                if ( rc == 0 )
                    VectorReorder ( remote_repositories, KRepositorySort, NULL );
            }

            if ( rc != 0 )
                KRepositoryVectorWhack ( remote_repositories );
        }
    }

    return rc;
}


/* CurrentProtectedRepository
 *  returns the currently active user protected repository
 */
LIB_EXPORT rc_t CC KRepositoryMgrCurrentProtectedRepository ( const KRepositoryMgr *self, const KRepository **protected )
{
    rc_t rc;

    if ( protected == NULL )
        rc = RC ( rcKFG, rcMgr, rcAccessing, rcParam, rcNull );
    else
    {
        * protected = NULL;

        if ( self == NULL )
            rc = RC ( rcKFG, rcMgr, rcAccessing, rcSelf, rcNull );
        else
        {
            KRepositoryVector v;
            rc = KRepositoryMgrUserRepositories ( self, & v );
            if ( rc == 0 )
            {
                KDirectory *wd;
                rc = KDirectoryNativeDir ( & wd );
                if ( rc == 0 )
                {
                    /* we need services of system directory */
                    struct KSysDir *sysDir = KDirectoryGetSysDir ( wd );

                    /* allocate buffer space for 3 paths */
                    const size_t path_size = 4096;
                    char *wd_path = malloc ( path_size * 3 );
                    if ( wd_path == NULL )
                        rc = RC ( rcKFG, rcMgr, rcAccessing, rcMemory, rcExhausted );
                    else
                    {
                        /* the working directory already has a canonical path */
                        rc = KDirectoryResolvePath ( wd, true, wd_path, path_size, "." );
                        if ( rc == 0 )
                        {
                            size_t wd_size = string_size ( wd_path );

                            /* look for all protected user repositories */
                            uint32_t i, count = VectorLength ( & v );
                            for ( i = 0; i < count; ++ i )
                            {
                                const KRepository *r = ( const void* ) VectorGet ( & v, i );
                                if ( r -> subcategory == krepProtectedSubCategory )
                                {
                                    size_t resolved_size;
                                    char *resolved = wd_path + path_size;

                                    /* get stated root path to repository */
                                    char *root = resolved + path_size;
                                    rc = KRepositoryRoot ( r, root, path_size, NULL );
                                    if ( rc != 0 )
                                        break;

                                    /* get its canonical path */
                                    rc = KSysDirRealPath ( sysDir, resolved, path_size, root );
                                    if ( rc != 0 )
                                        break;

                                    /* we know the current directory's canonical path size
                                       and we know the repository's canonical path size.
                                       to be "within" the repository, the current directory's
                                       size must be >= repository path size, and must match
                                       exactly the repository path itself over those bytes. */
                                    resolved_size = string_size ( resolved );
                                    if ( resolved_size <= wd_size && memcmp ( wd_path, resolved, resolved_size ) == 0 )
                                    {
                                        /* still have a little more to check */
                                        if ( resolved_size == wd_size ||
                                             wd_path [ resolved_size ] == '/' )
                                        {
                                            /* we are in the repository */
                                            rc = KRepositoryAddRef ( r );
                                            if ( rc == 0 )
                                                * protected = r;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        free ( wd_path );
                    }

                    KDirectoryRelease ( wd );
                }

                KRepositoryVectorWhack ( & v );
            }

            if ( rc == 0 && * protected == NULL )
                rc = RC ( rcKFG, rcMgr, rcAccessing, rcNode, rcNotFound );
        }
    }

    return rc;
}
