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

#include <vdb/extern.h>

#include <va_copy.h>

#define TRACK_REFERENCES 0

#define KONST const
#define SKONST
#include "cursor-priv.h"
#include "dbmgr-priv.h"
#include "linker-priv.h"
#include "table-priv.h"
#include "schema-priv.h"
#include "schema-parse.h"
#include "column-priv.h"
#include "phys-priv.h"
#include "prod-priv.h"
#include "prod-expr.h"
#undef KONST
#undef SKONST
#include "blob-priv.h"
#include "page-map.h"

#include <vdb/cursor.h>
#include <vdb/table.h>
#include <vdb/vdb-priv.h>
#include <kdb/table.h>
#include <kdb/meta.h>
#include <kdb/namelist.h>
#include <kfs/dyload.h>
#include <klib/symbol.h>
#include <klib/symtab.h>
#include <klib/namelist.h>
#include <klib/log.h>
#include <klib/rc.h>
#include <bitstr.h>
#include <os-native.h>
#include <sysalloc.h>

#include <kproc/lock.h>
#include <kproc/cond.h>
#include <kproc/thread.h>


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>


#define PERMIT_POST_OPEN_ADD 1
#define DISABLE_READ_CACHE 0


/*--------------------------------------------------------------------------
 * VCursorCache
 */

/* Whack
 */
void VCursorCacheWhack ( VCursorCache *self,
    void ( CC * whack ) ( void *item, void *data ), void *data )
{
    uint32_t i = VectorStart ( & self -> cache );
    uint32_t end = VectorLength ( & self -> cache );
    for ( end += i; i < end; ++ i )
    {
        Vector *ctx;
        rc_t rc = VectorSwap ( & self -> cache, i, NULL, ( void** ) & ctx );
        if ( rc == 0 && ctx != NULL )
        {
            VectorWhack ( ctx, whack, data );
            free ( ctx );
        }
    }

    VectorWhack ( & self -> cache, NULL, NULL );
}


/* Get
 *  retrieve object by cid
 */
void *VCursorCacheGet ( const VCursorCache *self, const VCtxId *cid )
{
    const Vector *ctx = ( const void* ) VectorGet ( & self -> cache, cid -> ctx );
    return VectorGet ( ctx, cid -> id );
}

/* Set
 *  set object by cid
 */
rc_t VCursorCacheSet ( VCursorCache *self, const VCtxId *cid, const void *item )
{
    Vector *ctx = VectorGet ( & self -> cache, cid -> ctx );
    if ( ctx == NULL )
    {
        rc_t rc;

        ctx = malloc ( sizeof * ctx );
        if ( ctx == NULL )
            return RC ( rcVDB, rcVector, rcConstructing, rcMemory, rcExhausted );

        rc = VectorSet ( & self -> cache, cid -> ctx, ctx );
        if ( rc != 0 )
        {
            free ( ctx );
            return rc;
        }

        VectorInit ( ctx, 0, 16 );
    }

    return VectorSet ( ctx, cid -> id, item );
}

/* Swap
 *  swap object by cid
 */
rc_t VCursorCacheSwap ( VCursorCache *self, const VCtxId *cid, const void *item, void **prior )
{
    Vector *ctx = VectorGet ( & self -> cache, cid -> ctx );
    if ( ctx == NULL )
    {
        * prior = NULL;
        return VCursorCacheSet ( self, cid, item );
    }
    return VectorSwap ( ctx, cid -> id, item, prior );
}


/*--------------------------------------------------------------------------
 * NamedParamNode
 */

typedef struct NamedParamNode NamedParamNode;
struct NamedParamNode
{
    BSTNode n;
    String name;
    KDataBuffer value;
};

static
void CC NamedParamNodeWhack ( BSTNode *n, void *ignore )
{
    NamedParamNode *self = ( NamedParamNode* ) n;
    KDataBufferWhack ( & self -> value );
    free ( self );
}

static
int CC NamedParamComp ( const void *item, const BSTNode *n )
{
    const String *name = item;
    const NamedParamNode *node = ( const NamedParamNode* ) n;

    return StringOrderNoNullCheck ( name, & node -> name );
}

static
int CC NamedParamNodeComp ( const BSTNode *A, const BSTNode *B )
{
    const NamedParamNode *a = (const NamedParamNode *) A;
    const NamedParamNode *b = (const NamedParamNode *) B;

    return StringOrderNoNullCheck ( & a -> name, & b -> name );
}
/*--------------------------------------------------------------------------
 * LinkedCursorNode
 */

typedef struct LinkedCursorNode LinkedCursorNode;
struct LinkedCursorNode
{
    BSTNode n;
    char tbl[64];
    VCursor *curs;
};

static
void CC LinkedCursorNodeWhack ( BSTNode *n, void *ignore )
{
    LinkedCursorNode *self = ( LinkedCursorNode* ) n;
    VCursorRelease (  self -> curs );
    free ( self );
}

static
int CC LinkedCursorComp ( const void *item, const BSTNode *n )
{
    const char *tbl = item;
    const LinkedCursorNode *node = ( const LinkedCursorNode* ) n;

    return strncmp ( tbl, node -> tbl, sizeof(node -> tbl) );
}

static
int CC LinkedCursorNodeComp ( const BSTNode *A, const BSTNode *B )
{
    const LinkedCursorNode *a = (const LinkedCursorNode *) A;
    const LinkedCursorNode *b = (const LinkedCursorNode *) B;

    return strncmp ( a -> tbl, b -> tbl,sizeof(a->tbl) );
}



/*--------------------------------------------------------------------------
 * VCursor
 *  a row cursor onto a VTable
 */

static void CC VCursorVColumnWhack_checked( void *item, void *data )
{
    if ( item != NULL )
        VColumnWhack( item, data );
}

/* Whack
 */
rc_t VCursorDestroy ( VCursor *self )
{
    KRefcountWhack ( & self -> refcount, "VCursor" );
    VBlobMRUCacheDestroy ( self->blob_mru_cache);
    if ( self -> user_whack != NULL )
        ( * self -> user_whack ) ( self -> user );
    BSTreeWhack ( & self -> named_params, NamedParamNodeWhack, NULL );
    BSTreeWhack ( & self -> linked_cursors, LinkedCursorNodeWhack, NULL );
    VCursorCacheWhack ( & self -> col, NULL, NULL );
    VCursorCacheWhack ( & self -> phys, VPhysicalWhack, NULL );
    VCursorCacheWhack ( & self -> prod, NULL, NULL );
    VectorWhack ( & self -> owned, VProductionWhack, NULL );
    VectorWhack ( & self -> trig, NULL, NULL );
    VectorWhack ( & self -> row, VCursorVColumnWhack_checked, NULL );

    VSchemaRelease ( self -> schema );

    VTableSever ( self -> tbl );

    free ( self );

    return 0;
}


/* AddRef
 * Release
 *  all objects are reference counted
 *  NULL references are ignored
 */
LIB_EXPORT rc_t CC VCursorAddRef ( const VCursor *self )
{
    if ( self != NULL )
    {
        switch ( KRefcountAdd ( & self -> refcount, "VCursor" ) )
        {
        case krefLimit:
            return RC ( rcVDB, rcCursor, rcAttaching, rcRange, rcExcessive );
        }
    }
    return 0;
}

LIB_EXPORT rc_t CC VCursorRelease ( const VCursor *self )
{
    if ( self != NULL )
    {
        switch ( KRefcountDrop ( & self -> refcount, "VCursor" ) )
        {
        case krefWhack:
            return VCursorWhack ( ( VCursor* ) self );
        case krefLimit:
            return RC ( rcVDB, rcCursor, rcReleasing, rcRange, rcExcessive );
        }
    }
    return 0;
}

/* Make - PRIVATE
 */
rc_t VCursorMake ( VCursor **cursp, const VTable *tbl )
{
    rc_t rc;
    VCursor *curs;

    /* must have return param */
    assert ( cursp != NULL );

    /* must have parent tbl */
    assert ( tbl != NULL );

    /* create a structure */
    curs = calloc ( 1, sizeof * curs );
    if ( curs == NULL )
        rc = RC ( rcVDB, rcCursor, rcConstructing, rcMemory, rcExhausted );
    else
    {
        /* create a separate schema object */
        rc = VSchemaMake ( & curs -> schema, tbl -> schema );
        if ( rc == 0 )
        {
            /* extend table schema to populate with implicits */
            rc = STableCloneExtend ( tbl -> stbl, & curs -> stbl, curs -> schema );
            if ( rc == 0 )
            {
                curs -> tbl = VTableAttach ( tbl );
                VectorInit ( & curs -> row, 1, 16 );
                VCursorCacheInit ( & curs -> col, 0, 16 );
                VCursorCacheInit ( & curs -> phys, 0, 16 );
                VCursorCacheInit ( & curs -> prod, 0, 16 );
                VectorInit ( & curs -> owned, 0, 64 );
                VectorInit ( & curs -> trig, 0, 64 );
                KRefcountInit ( & curs -> refcount, 1, "VCursor", "make", "vcurs" );
                curs -> state = vcConstruct;
                curs -> permit_add_column = true;
		curs -> suspend_triggers  = false;
                * cursp = curs;
                return 0;
            }

            VSchemaRelease ( curs -> schema );
        }

        free ( curs );
    }

    * cursp = NULL;

    return rc;
}

/* SupplementSchema
 *  scan table for physical column names
 *  create transparent yet incomplete (untyped) columns for unknown names
 *  create incomplete (untyped) physical columns for forwarded names
 *  repeat process on static columns, except create complete (fully typed) objects
 */
static
rc_t VCursorSupplementName ( const KSymTable *tbl,
    STable *stbl, const VTypedecl *td, const char *name )
{
    rc_t rc = 0;
    char buffer [ 256 ];

    /* create physical name string */
    int len = snprintf ( buffer, sizeof buffer, ".%s", name );
    if ( len < 0 || len >= sizeof buffer )
        rc = RC ( rcVDB, rcCursor, rcConstructing, rcName, rcExcessive );
    else
    {
        KSymbol *sym;

        String pname, cname;
        StringInit ( & pname, buffer, len, string_len ( buffer, len ) );

        /* if physical name is known */
        sym = KSymTableFind ( tbl, & pname );
        if ( sym != NULL )
        {
            /* if it is being implemented here */
            if ( sym -> type == eVirtual )
                rc = STableImplicitPhysMember ( stbl, td, sym, & pname );
            return rc;
        }

        /* if simple name is unknown, add implicit */
        sym = KSymTableFind ( tbl, StringSubstr ( & pname, & cname, 1, 0 ) );
        if ( sym == NULL )
        {
            /* create implicit physical */
            rc = STableImplicitPhysMember ( stbl, td, sym, & pname );
            if ( rc == 0 )
                rc = STableImplicitColMember ( stbl, & cname, & pname );
        }
    }
    return rc;
}

static
rc_t VCursorSupplementPhysical ( const KSymTable *tbl, const VCursor *self )
{
    KNamelist *names;
    rc_t rc = KTableListCol ( self -> tbl -> ktbl, & names );
    if ( rc == 0 )
    {
        uint32_t i, count;
        rc = KNamelistCount ( names, & count );
        for ( i = 0; rc == 0 && i < count; ++ i )
        {
            const char *name;
            rc = KNamelistGet ( names, i, & name );
            if ( rc == 0 )
                rc = VCursorSupplementName ( tbl, self -> stbl, NULL, name );
        }
        KNamelistRelease ( names );
    }
    return rc;
}

static
rc_t VCursorSupplementStatic ( const KSymTable *tbl, const VCursor *self )
{
    rc_t rc;
    KNamelist *names;

    const KMDataNode *root = self -> tbl -> col_node;
    if ( root == NULL )
        return 0;

    rc = KMDataNodeListChild ( root, & names );
    if ( rc == 0 )
    {
        uint32_t i, count;
        rc = KNamelistCount ( names, & count );
        for ( i = 0; rc == 0 && i < count; ++ i )
        {
            const char *name;
            rc = KNamelistGet ( names, i, & name );
            if ( rc == 0 )
            {
                const KMDataNode *node;
                rc = KMDataNodeOpenNodeRead ( root, & node, name );
                if ( rc == 0 )
                {
                    size_t size;
                    char typedecl [ 256 ];
                    rc = KMDataNodeReadAttr ( node, "type", typedecl, sizeof typedecl, & size );
                    if ( rc == 0 && size != 0 )
                    {
                        VTypedecl td;
                        rc = VSchemaResolveTypedecl ( self -> schema, & td, typedecl );
                        if ( rc == 0 )
                            rc = VCursorSupplementName ( tbl, self -> stbl, & td, name );

                        rc = 0; /*** don't care if name is not in the schema ***/
		
                    }

                    KMDataNodeRelease ( node );
                }
            }
        }

        KNamelistRelease ( names );
    }

    return rc;
}

rc_t VCursorSupplementSchema ( const VCursor *self )
{
    KSymTable tbl;
    rc_t rc = init_tbl_symtab ( & tbl, self -> schema, self -> stbl );
    if ( rc == 0 )
    {
        rc = VCursorSupplementPhysical ( & tbl, self );
        if ( rc == 0 )
            rc = VCursorSupplementStatic ( & tbl, self );
        KSymTableWhack ( & tbl );
    }
    return rc;
}


/* CreateCachedCursorRead
 *  creates a read cursor object onto table with a cache limit in bytes
 *
 *  AVAILABILITY: version 2.1
 *
 *  "curs" [ OUT ] - return parameter for newly created cursor
 *
 *  "capacity" [ IN ] - the maximum bytes to cache on the cursor before
 *  dropping least recently used blobs
 */
static rc_t VTableCreateCachedCursorReadImpl(const VTable *self, const VCursor **cursp, size_t capacity,bool create_pagemap_thread);
LIB_EXPORT rc_t CC VTableCreateCachedCursorRead ( const VTable *self,
    const VCursor **cursp, size_t capacity )
{
	return VTableCreateCachedCursorReadImpl(self,cursp,capacity,true);
}
/**
*** VTableCreateCursorReadInternal is only visible in vdb and needed for schema resolutions
****/
rc_t  VTableCreateCursorReadInternal(const VTable *self, const VCursor **cursp)
{
	return VTableCreateCachedCursorReadImpl(self,cursp,0,false);
}
static rc_t VTableCreateCachedCursorReadImpl ( const VTable *self,
    const VCursor **cursp, size_t capacity, bool create_pagemap_thread  )
{
    rc_t rc;
#if DISABLE_READ_CACHE
    capacity = 0;
#endif
    if ( cursp == NULL )
        rc = RC ( rcVDB, rcTable, rcOpening, rcParam, rcNull );
    else {
        if ( self == NULL )
            rc = RC ( rcVDB, rcTable, rcOpening, rcSelf, rcNull );
        else {
            VCursor *curs;
#if LAZY_OPEN_COL_NODE
            if ( self -> col_node == NULL )
                KMetadataOpenNodeRead ( self -> meta, & ( ( VTable* ) self ) -> col_node, "col" );
#endif
            rc = VCursorMake ( & curs, self );
            if ( rc == 0 ) {
		curs -> blob_mru_cache = VBlobMRUCacheMake(capacity);
                curs -> read_only = true;
                rc = VCursorSupplementSchema ( curs );
               
#if 0  
		if(create_pagemap_thread && capacity > 0 && rc == 0 )
			rc = VCursorLaunchPagemapThread ( curs );
#endif
                if ( rc == 0 )
                {
		    if(capacity > 0)
			curs->launch_cnt = 5;
		    else
			curs->launch_cnt=200;
                    * cursp = curs;
                    return 0;
                }
                VCursorRelease ( curs );
            }
        }
        * cursp = NULL;
    }
    return rc;
}

/* CreateCursor
 *  creates a cursor object onto table
 *  multiple read cursors are allowed
 *  only a single write cursor is allowed
 *
 *  "curs" [ OUT ] - return parameter for newly created cursor
 */
LIB_EXPORT rc_t CC VTableCreateCursorRead ( const VTable *self, const VCursor **curs )
{
    /* will be deprecated in the future */
    return VTableCreateCachedCursorRead ( self, curs, 0 );
}

/* PermitPostOpenAdd
 *  allows columns to be added to open cursor
 *  for write cursor, the effect lasts until the first row commit
 */
LIB_EXPORT rc_t CC VCursorPermitPostOpenAdd ( const VCursor *cself )
{
    rc_t rc;
    VCursor *self = ( VCursor* ) cself;

    if ( self == NULL )
        rc = RC ( rcVDB, rcCursor, rcUpdating, rcSelf, rcNull );
    else if ( self -> state == vcFailed )
        rc = RC ( rcVDB, rcCursor, rcUpdating, rcCursor, rcInvalid );
    else if ( self -> state != vcConstruct )
        rc = RC ( rcVDB, rcCursor, rcUpdating, rcCursor, rcOpen );
    else
    {
        self -> permit_post_open_add = true;
        rc = 0;
    }

    return rc;
}
/*  SuspendTriggers
 *  blocks resolution of schema-based triggers
 *  
 */
LIB_EXPORT rc_t CC VCursorSuspendTriggers ( const VCursor *cself )
{
    rc_t rc;
    VCursor *self = ( VCursor* ) cself;

    if ( self == NULL )
        rc = RC ( rcVDB, rcCursor, rcUpdating, rcSelf, rcNull );
    else
    {
        self -> suspend_triggers = true;
        rc = 0;
    }

    return rc;
}


/* AddSColumn
 */
static
rc_t VCursorAddSColumn ( VCursor *self, uint32_t *idx, const SColumn *scol, const VTypedecl *cast )
{
    rc_t rc;
    VColumn *col;

    if ( self -> read_only )
    {
        /* must be readable */
        if ( scol -> read == NULL )
            return RC ( rcVDB, rcCursor, rcUpdating, rcColumn, rcWriteonly );
    }
    else
    {
        /* must be writable */
        if ( scol -> read_only || ( scol -> read == NULL && scol -> validate == NULL ) )
            return RC ( rcVDB, rcCursor, rcUpdating, rcColumn, rcReadonly );
    }

    /* must not already be there - benign error */
    col = VCursorCacheGet ( & self -> col, & scol -> cid );
    if ( col != NULL )
    {
        * idx = col -> ord;
        return RC ( rcVDB, rcCursor, rcUpdating, rcColumn, rcExists );
    }

    /* make object */
    rc = VCursorMakeColumn ( self, & col, scol );
    if ( rc == 0 )
    {
        /* insert it into vectors */
        rc = VectorAppend ( & self -> row, & col -> ord, col );
        if ( rc == 0 )
        {
            void *ignore;
            rc = VCursorCacheSet ( & self -> col, & scol -> cid, col );
            if ( rc == 0 )
            {
                /* open column if cursor open or type unknown */
                if ( self -> state >= vcReady || scol -> td . type_id == 0 )
                {
                    rc = VCursorPostOpenAdd ( self, col );
                    assert ( rc != 0 || scol -> td . type_id != 0 );
                }
                if ( rc == 0 )
                {
                    /* check cast of SColumn against requested type
                       this is to handle the case where the column
                       was created incomplete, i.e. with unknown type */
                    if ( cast == NULL || VTypedeclToTypedecl ( & scol -> td,
                             self -> schema, cast, & col -> td, NULL ) )
                    {
                        /* has been entered */
                        * idx = col -> ord;
                        return 0;
                    }
                }

                /* bail out */
                VCursorCacheSwap ( & self -> col, & scol -> cid, NULL, & ignore );
            }

            VectorSwap ( & self -> row, col -> ord, NULL, & ignore );
        }

        VColumnWhack ( col, NULL );
    }

    return rc;
}


/* AddColspec
 *  a "colspec" is either a simple column name or a typed name expression
 *  uses STable to evaluate colspec and find an SColumn
 */
static
rc_t VCursorAddColspec ( VCursor *self, uint32_t *idx, const char *colspec )
{
    rc_t rc;

    /* find an appropriate column in schema */
    uint32_t type;
    VTypedecl cast;
    const SNameOverload *name;
    const SColumn *scol = STableFind ( self -> tbl -> stbl, self -> schema,
        & cast, & name, & type, colspec, "VCursorAddColspec", true );
    if ( scol == NULL || type != eColumn )
        rc = RC ( rcVDB, rcCursor, rcUpdating, rcColumn, rcNotFound );
    else
        rc = VCursorAddSColumn ( self, idx, scol, & cast );

    return rc;
}


/* AddColumn
 *  add a column to an unopened cursor
 *
 *  "idx" [ OUT ] - return parameter for column index
 *
 *  "name" [ IN ] - NUL terminated column name spec.
 *  to identify a column by name, provide the column name
 *  by itself. if there are multiple types available under
 *  that name, the default type for that column will be
 *  selected. to select a specific type, the name may
 *  be cast to that type using a cast expression, e.g.
 *    "( type ) name"
 *  the special name "*" may be added to a read cursor.
 */
LIB_EXPORT rc_t CC VCursorVAddColumn ( const VCursor *cself,
    uint32_t *idx, const char *name, va_list args )
{
    rc_t rc;
    VCursor *self = ( VCursor* ) cself;

    if ( idx == NULL )
        rc = RC ( rcVDB, rcCursor, rcUpdating, rcParam, rcNull );
    else
    {
        * idx = 0;

        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcUpdating, rcSelf, rcNull );
        else if ( name == NULL )
            rc = RC ( rcVDB, rcCursor, rcUpdating, rcName, rcNull );
        else if ( name [ 0 ] == 0 )
            rc = RC ( rcVDB, rcCursor, rcUpdating, rcName, rcEmpty );
        else if ( self -> state == vcFailed )
            rc = RC ( rcVDB, rcCursor, rcUpdating, rcCursor, rcInvalid );
        else if ( self -> state != vcConstruct && ! self -> permit_add_column )
            rc = RC ( rcVDB, rcCursor, rcUpdating, rcCursor, rcLocked );
        else
        {
            char colspec [ 1024 ];
            int len = vsnprintf ( colspec, sizeof colspec, name, args );
            if ( len < 0 || len >= sizeof colspec )
                rc = RC ( rcVDB, rcCursor, rcUpdating, rcName, rcExcessive );
            else
            {
                rc = VCursorAddColspec ( self, idx, colspec );
                if ( rc == 0 || GetRCState ( rc ) == rcExists )
                    return rc;
            }

            if ( ! self -> permit_add_column )
            {
                PLOGERR ( klogErr, ( klogErr, rc, "failed to add column '$(spec)' to cursor",
                                     "spec=%s", colspec ));
            }
            return rc;
        }
    }

    LOGERR ( klogErr, rc, "failed to add column" );

    return rc;
}


/* AddColumn
 *  add a column to an unopened cursor
 *
 *  "idx" [ OUT ] - return parameter for column index
 *
 *  "name" [ IN ] - NUL terminated column name spec.
 *  to identify a column by name, provide the column name
 *  by itself. if there are multiple types available under
 *  that name, the default type for that column will be
 *  selected. to select a specific type, the name may
 *  be cast to that type using a cast expression, e.g.
 *    "( type ) name"
 *  the special name "*" may be added to a read cursor.
 */
LIB_EXPORT rc_t CC VCursorAddColumn ( const VCursor *self,
    uint32_t *idx, const char *name, ... )
{
    rc_t rc;
    va_list args;

    va_start ( args, name );
    rc = VCursorVAddColumn ( self, idx, name, args );
    va_end ( args );

    return rc;
}


/* GetColspec
 *  a "colspec" is either a simple column name or a typed name expression
 *  uses STable to evaluate colspec and find an SColumn
 */
static
rc_t VCursorGetColspec ( const VCursor *self, uint32_t *idx, const char *colspec )
{
    rc_t rc;

    /* find an appropriate column in schema */
    uint32_t type;
    VTypedecl cast;
    const SNameOverload *name;
    const SColumn *scol = STableFind ( self -> tbl -> stbl, self -> schema,
        & cast, & name, & type, colspec, "VCursorAddColspec", true );
    if ( scol == NULL || type != eColumn )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcColumn, rcNotFound );
    else
    {
        /* if the column-spec gave us the exact column, return it */
        VColumn *col = VCursorCacheGet ( & self -> col, & scol -> cid );
        if ( col != NULL )
        {
            * idx = col -> ord;
            return 0;
        }

        /* prepare for failure */
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcColumn, rcNotFound );

        /* there should be a name overload object */
        if ( name != NULL )
        {
            uint32_t count, i = VectorStart ( & name -> items );
            uint32_t end = VectorLength ( & name -> items );
            for ( end += i, count = 0; i < end; ++ i )
            {
                scol = ( const void* ) VectorGet ( & name -> items, i );
                if ( scol != NULL )
                {
                    col = VCursorCacheGet ( & self -> col, & scol -> cid );
                    if ( col != NULL )
                    {
                        * idx = col -> ord;
                        ++ count;
                    }
                }
            }

            if ( count == 1 )
                return 0;
            if ( count != 0 )
                return RC ( rcVDB, rcCursor, rcAccessing, rcColumn, rcAmbiguous );
        }
    }

    return rc;
}


/* GetColumnIdx
 *  retrieve column index by name spec
 *
 *  "idx" [ OUT ] - return parameter for column index
 *
 *  "name" [ IN ] - NUL terminated column name spec.
 */
LIB_EXPORT rc_t CC VCursorVGetColumnIdx ( const VCursor *self,
    uint32_t *idx, const char *name, va_list args )
{
    rc_t rc;

    if ( idx == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        * idx = 0;

        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else if ( name == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcName, rcNull );
        else if ( name [ 0 ] == 0 )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcName, rcEmpty );
        else if ( self -> state == vcFailed )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcCursor, rcInvalid );
        else
        {
            char colspec [ 1024 ];
            int len = vsnprintf ( colspec, sizeof colspec, name, args );
            if ( len < 0 || len >= sizeof colspec )
                rc = RC ( rcVDB, rcCursor, rcAccessing, rcName, rcExcessive );
            else
            {
                rc = VCursorGetColspec ( self, idx, colspec );
            }
        }
    }

    return rc;
}

LIB_EXPORT rc_t CC VCursorGetColumnIdx ( const VCursor *self, uint32_t *idx, const char *name, ... )
{
    rc_t rc;
    va_list args;

    va_start ( args, name );
    rc = VCursorVGetColumnIdx ( self, idx, name, args );
    va_end ( args );

    return rc;
}


/* Datatype
 *  returns typedecl and/or typedef for column data
 *
 *  "idx" [ IN ] - column index
 *
 *  "type" [ OUT, NULL OKAY ] - returns the column type declaration
 *
 *  "def" [ OUT, NULL OKAY ] - returns the definition of the type
 *  returned in "type_decl"
 *
 * NB - one of "type" and "def" must be non-NULL
 */
LIB_EXPORT rc_t CC VCursorDatatype ( const VCursor *self, uint32_t idx,
    struct VTypedecl *type, struct VTypedesc *desc )
{
    rc_t rc;

    if ( type == NULL && desc == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else
        {
            const VColumn *vcol = ( const VColumn* ) VectorGet ( & self -> row, idx );
            if ( vcol == NULL )
                rc = RC ( rcVDB, rcCursor, rcAccessing, rcColumn, rcNotFound );
            else
                return VColumnDatatype ( vcol, type, desc );
        }

        if ( type != NULL )
            memset ( type, 0, sizeof * type );
        if ( desc != NULL )
            memset ( desc, 0, sizeof * desc );
    }

    return rc;
}


/* IdRange
 *  returns id range for column
 *
 *  "idx" [ IN ] - column index
 *
 *  "id" [ IN ] - page containing this row id is target
 *
 *  "first" [ OUT, NULL OKAY ] and "last" [ OUT, NULL OKAY ] -
 *  id range is returned in these output parameters, where
 *  at least ONE must be NOT-NULL
 */
typedef struct VCursorIdRangeData VCursorIdRangeData;
struct VCursorIdRangeData
{
    int64_t first, last;
    rc_t rc;
};

static
bool CC column_id_range ( void *item, void *data )
{
    if ( ( size_t ) item > 8 )
    {
        int64_t first, last;
        VCursorIdRangeData *pb = data;

        rc_t rc = VColumnIdRange ( ( const void* ) item, & first, & last );

        if ( GetRCState ( rc ) == rcEmpty )
            return false;

        if ( ( pb -> rc = rc ) != 0 )
            return true;

        if ( first < pb -> first )
            pb -> first = first;
        if ( last > pb -> last )
            pb -> last = last;
    }

    return false;
}

LIB_EXPORT rc_t CC VCursorIdRange ( const VCursor *self, uint32_t idx,
    int64_t *first, uint64_t *count )
{
    rc_t rc;

    if ( first == NULL && count == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        int64_t dummy;
        uint64_t dummy_count;

        if ( first == NULL )
            first = & dummy;
        else if ( count == NULL )
            count = & dummy_count;

        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else if ( self -> state < vcReady )
        {
            if ( self -> state == vcFailed )
                rc = RC ( rcVDB, rcCursor, rcAccessing, rcCursor, rcInvalid );
            else
                rc = RC ( rcVDB, rcCursor, rcAccessing, rcCursor, rcNotOpen );
        }
        else if ( idx == 0 )
        {
            VCursorIdRangeData pb;

            pb . first = INT64_MAX;
            pb . last = INT64_MIN;
            pb . rc = RC ( rcVDB, rcCursor, rcAccessing, rcRange, rcEmpty );

            if ( ! VectorDoUntil ( & self -> row, false, column_id_range, & pb ) )
            {
                * first = pb . first;
                * count = pb . last >= pb . first ? pb . last + 1 - pb . first : 0;
                return pb . rc;
            }

            rc = pb . rc;
        }
        else
        {
            const VColumn *vcol = ( const VColumn* ) VectorGet ( & self -> row, idx );
            if ( vcol == NULL )
                rc = RC ( rcVDB, rcCursor, rcAccessing, rcColumn, rcNotFound );
            else {
                int64_t last;

                rc = VColumnIdRange ( vcol, first, &last );
                if (rc == 0)
                    *count = last + 1 - *first;
                return rc;
            }
        }

        * first = * count = 0;
    }

    return rc;
}

LIB_EXPORT rc_t CC VCursorPageIdRange ( struct VCursor const *self,
    uint32_t idx, int64_t id, int64_t *first, int64_t *last )
{
    rc_t rc;

    if ( first == NULL && last == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        int64_t dummy;
        if ( first == NULL )
            first = & dummy;
        else if ( last == NULL )
            last = & dummy;

        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else
        {
            const VColumn *vcol = ( const VColumn* ) VectorGet ( & self -> row, idx );
            if ( vcol == NULL )
                rc = RC ( rcVDB, rcCursor, rcAccessing, rcColumn, rcNotFound );
            else
                return VColumnPageIdRange ( vcol, id, first, last );
        }

        * first = * last = 0;
    }

    return rc;
}


/* Open
 *  open cursor, resolving schema
 *  for the set of opened columns
 *
 *  NB - there is no corresponding "Close"
 *  use "Release" instead.
 */
typedef struct VProdResolveData VProdResolveData;
struct VProdResolveData
{
    VProdResolve pr;
    rc_t rc;
};


static
bool CC VCursorResolveColumn ( void *item, void *data )
{
    if ( item != NULL )
    {
        void *ignore;
        VCursor *self;

        VColumn *col = item;
        VProdResolveData *pb = data;
        SColumn *scol = ( SColumn* ) col -> scol;

        VProduction *src = NULL;
        pb -> rc = VProdResolveColumnRoot ( & pb -> pr, & src, scol );
        if ( pb -> rc == 0 )
        {
            if ( src > FAILED_PRODUCTION )
            {
                /* repair for incomplete implicit column decl */
                if ( scol -> td . type_id == 0 )
                    scol -> td = src -> fd . td;

                return false;
            }

            pb -> rc = RC ( rcVDB, rcCursor, rcOpening, rcColumn, rcUndefined );
        }

        /* check for tolerance */
        self = pb -> pr . curs;
        if ( ! pb -> pr . ignore_column_errors )
        {
            if ( ! self -> permit_post_open_add )
            {
                PLOGERR ( klogErr, ( klogErr, pb -> rc, "failed to resolve column '$(name)' idx '$(idx)'",
                                     "name=%.*s,idx=%u"
                                     , ( int ) scol -> name -> name . size
                                     , scol -> name -> name . addr
                                     , col -> ord ));
            }

            return true;
        }

        /* remove from row and cache */
        VectorSwap ( & self -> row, col -> ord, NULL, & ignore );
        VCursorCacheSwap ( & self -> col, & scol -> cid, NULL, & ignore );

        /* dump the VColumn */
        VColumnWhack ( col, NULL );

        /* return no-error */
        pb -> rc = 0;
    }

    return false;
}

static
rc_t VCursorOpenColumn ( const VCursor *cself, VColumn *col )
{
    KDlset *libs;
    VCursor *self = ( VCursor* ) cself;

    VProdResolveData pb;
    pb . pr . schema = self -> schema;
    pb . pr . ld = self -> tbl -> linker;
    pb . pr . stbl = self -> stbl;
    pb . pr . curs = self;
    pb . pr . cache = & self -> prod;
    pb . pr . owned = & self -> owned;
    pb . pr . chain = chainDecoding;
    pb . pr . blobbing = false;
    pb . pr . ignore_column_errors = false;
    pb . pr . discover_writable_columns = false;

    pb . rc = VLinkerOpen ( pb . pr . ld, & libs );
    if ( pb . rc == 0 )
    {
        pb . pr . libs = libs;
        VCursorResolveColumn ( col, & pb );
        KDlsetRelease ( libs );
    }

    return pb . rc;
}

/* PostOpenAdd
 *  handle opening of a column after the cursor is opened
 */
rc_t VCursorPostOpenAddRead ( VCursor *self, VColumn *col )
{
    return VCursorOpenColumn ( self, col );
}


static
rc_t VCursorResolveColumnProductions ( VCursor *self,
    const KDlset *libs, bool ignore_failures )
{
    VProdResolveData pb;
    pb . pr . schema = self -> schema;
    pb . pr . ld = self -> tbl -> linker;
    pb . pr . libs = libs;
    pb . pr . stbl = self -> stbl;
    pb . pr . curs = self;
    pb . pr . cache = & self -> prod;
    pb . pr . owned = & self -> owned;
    pb . pr . chain = chainDecoding;
    pb . pr . blobbing = false;
    pb . pr . ignore_column_errors = ignore_failures;
    pb . pr . discover_writable_columns = false;
    pb . rc = 0;

    if ( VectorDoUntil ( & self -> row, false, VCursorResolveColumn, & pb ) )
        return pb . rc;

    return 0;
}

rc_t VCursorOpenRead ( VCursor *self, const KDlset *libs )
{
    rc_t rc;

    if ( self -> state >= vcReady )
        rc = 0;
    else if ( self -> state == vcFailed )
        rc = RC ( rcVDB, rcCursor, rcOpening, rcCursor, rcInvalid );
    else
    {
        rc = VCursorResolveColumnProductions ( self, libs, false );
        if ( rc == 0 )
        {
            self -> row_id = self -> start_id = self -> end_id = 1;
            self -> state = vcReady;
            return 0;
        }

        self -> state = vcFailed;
    }

    return rc;
}

static
rc_t VCursorOpenForListing ( const VCursor *cself )
{
    rc_t rc;
    VCursor *self = ( VCursor* ) cself;

    VLinker *ld = self -> tbl -> linker;

    KDlset *libs;
    rc = VLinkerOpen ( ld, & libs );
    if ( rc == 0 )
    {
        rc = VCursorResolveColumnProductions ( self, libs, true );
        KDlsetRelease ( libs );
    }
    return rc;
}



/* RowId
 *  return id from row cursor within open blob
 *
 *  "id" [ OUT ] - current row id
 */
LIB_EXPORT rc_t CC VCursorRowId ( const VCursor *self, int64_t *id )
{
    rc_t rc;
    if ( id == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else
        {
            * id = self -> row_id;
            return 0;
        }

        * id = 0;
    }

    return rc;
}


/* SetRowIdRead - PRIVATE
 *  seek to given row id
 *
 *  "row_id" [ IN ] - row id to select
 */
rc_t VCursorSetRowIdRead ( VCursor *self, int64_t row_id )
{
    assert ( self != NULL );

    self -> row_id = row_id;
    return 0;
}


/* OpenRowRead
 * CloseRowRead
 */
rc_t VCursorOpenRowRead ( VCursor *self )
{
    assert ( self != NULL );

    self -> state = vcRowOpen;
    return 0;
}

rc_t VCursorCloseRowRead ( VCursor *self )
{
    assert ( self != NULL );

    ++ self -> row_id;
    self -> state = vcReady;
    return 0;
}


/* Read
 *  read entire single row of byte-aligned data into a buffer
 *
 *  "col_idx" [ IN ] - index of column to be read, returned by "AddColumn"
 *
 *  "elem_bits" [ IN ] - expected element size in bits, required
 *  to be compatible with the actual element size, and be a multiple
 *  of 8 ( byte-aligned ). for non-byte-aligned data, see ReadBits
 *
 *  "buffer" [ OUT ] and "blen" [ IN ] - return buffer for row data
 *  where "blen" gives buffer capacity in elements. the total buffer
 *  size in bytes == ( "elem_bits" * "blen" + 7 ) / 8.
 *
 *  "row_len" [ OUT ] - return parameter for the number of elements
 *  in the requested row.
 *
 *  when the return code is 0, "row_len" will contain the number of
 *  elements read into buffer. if the return code indicates that the
 *  buffer is too small, "row_len" will give the required buffer length.
 */
static
rc_t VCursorReadColumnDirectInt ( const VCursor *cself, int64_t row_id, uint32_t col_idx,
    uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len,
    const VBlob **rslt )
{
    rc_t rc,rc_cache=0;
    const VColumn *col;
    const VBlob *blob;

    col = ( const void* ) VectorGet ( & cself -> row, col_idx );
    if ( col == NULL )
        return RC ( rcVDB, rcCursor, rcReading, rcColumn, rcInvalid );

    /* 2.0 behavior if not caching */
    if ( cself -> blob_mru_cache == NULL )
        return VColumnRead ( col, row_id, elem_bits, base, boff, row_len, (VBlob**) rslt );

    /* check MRU blob */
    blob = VBlobMRUCacheFind(cself->blob_mru_cache,col_idx,row_id);
    if(blob){
        /* ask column to read from blob */
	assert(row_id >= blob->start_id && row_id <= blob->stop_id);
        return VColumnReadCachedBlob ( col, blob, row_id, elem_bits, base, boff, row_len);
    }
    { /* ask column to produce a blob to be cached */
	VBlobMRUCacheCursorContext cctx;
	cctx.cache=cself -> blob_mru_cache;
	cctx.col_idx = col_idx;
	rc = VColumnReadBlob(col,&blob,row_id,elem_bits,base,boff,row_len,&cctx);
    }
    if ( rc != 0 || blob == NULL ){
	if(rslt) *rslt = NULL;
        return rc;
    }
    if(blob->stop_id > blob->start_id + 4)
	    rc_cache=VBlobMRUCacheSave(cself->blob_mru_cache, col_idx, blob);
    if(rslt==NULL){ /** user does not care about the blob ***/
	if( rc_cache == 0){
		VBlobRelease((VBlob*)blob);
	} /** else the memory will leak **/
    } else {
	*rslt=blob;
    }
    return 0;
}

/* GetBlob
 *  retrieve a blob of data containing the current row id
 * GetBlobDirect
 *  retrieve a blob of data containing the requested row id
 *
 *  "blob" [ OUT ] - return parameter for a new reference
 *  to VBlob containing requested cell. NB - must be released
 *  via VBlobRelease when no longer needed.
 *
 *  "row_id" [ IN ] - allows ReadDirect random access to any cell
 *  in column
 *
 *  "col_idx" [ IN ] - index of column to be read, returned by "AddColumn"
 */
LIB_EXPORT rc_t CC VCursorGetBlob ( const VCursor *self,
    const VBlob **blob, uint32_t col_idx )
{
    rc_t rc;

    if ( blob == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else if ( ! self -> read_only )
            rc = RC ( rcVDB, rcCursor, rcReading, rcCursor, rcWriteonly );
        else
        {
            const void *base;
            uint32_t elem_bits, boff, row_len;

            switch ( self -> state )
            {
            case vcConstruct:
                rc = RC ( rcVDB, rcCursor, rcReading, rcCursor, rcNotOpen );
                break;
            case vcReady:
                rc = RC ( rcVDB, rcCursor, rcReading, rcRow, rcNotOpen );
                break;
            case vcRowOpen:
                rc = VCursorReadColumnDirectInt
                    ( self, self -> row_id, col_idx, &elem_bits, &base, &boff, &row_len, blob );
                if ( rc == 0 )
                {
                    rc = VBlobAddRef ( ( VBlob* ) *blob );
                    if ( rc == 0 )
                        return 0;
                }
                break;
            default:
                rc = RC ( rcVDB, rcCursor, rcReading, rcCursor, rcInvalid );
            }
        }

        * blob = NULL;
    }
    return rc;
}

LIB_EXPORT rc_t CC VCursorGetBlobDirect ( const VCursor *self,
    const VBlob **blob, int64_t row_id, uint32_t col_idx )
{
    rc_t rc;

    if ( blob == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else if ( ! self -> read_only )
            rc = RC ( rcVDB, rcCursor, rcReading, rcCursor, rcWriteonly );
        else
        {
            const void *base;
            uint32_t elem_bits, boff, row_len;

            switch ( self -> state )
            {
            case vcConstruct:
                rc = RC ( rcVDB, rcCursor, rcReading, rcCursor, rcNotOpen );
                break;
            case vcReady:
            case vcRowOpen:
                rc = VCursorReadColumnDirectInt
                    ( self, row_id, col_idx, &elem_bits, &base, &boff, &row_len, blob );
                if ( rc == 0 )
                {
                    rc = VBlobAddRef ( ( VBlob* ) *blob );
                    if ( rc == 0 )
                        return 0;
                }
                break;
            default:
                rc = RC ( rcVDB, rcCursor, rcReading, rcCursor, rcInvalid );
            }
        }

        * blob = NULL;
    }
    return rc;
}

static
rc_t VCursorReadColumnDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx,
    uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len )
{
    if ( ! self -> read_only )
        return RC ( rcVDB, rcCursor, rcReading, rcCursor, rcWriteonly );

    switch ( self -> state )
    {
    case vcConstruct:
        return RC ( rcVDB, rcCursor, rcReading, rcCursor, rcNotOpen );
    case vcReady:
    case vcRowOpen:
        break;
    default:
        return RC ( rcVDB, rcCursor, rcReading, rcCursor, rcInvalid );
    }

    return VCursorReadColumnDirectInt
        ( self, row_id, col_idx, elem_bits, base, boff, row_len, NULL );
}


static
rc_t VCursorReadColumn ( const VCursor *self, uint32_t col_idx,
    uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len )
{
    if ( ! self -> read_only )
        return RC ( rcVDB, rcCursor, rcReading, rcCursor, rcWriteonly );

    switch ( self -> state )
    {
    case vcConstruct:
        return RC ( rcVDB, rcCursor, rcReading, rcCursor, rcNotOpen );
    case vcReady:
        return RC ( rcVDB, rcCursor, rcReading, rcRow, rcNotOpen );
    case vcRowOpen:
        break;
    default:
        return RC ( rcVDB, rcCursor, rcReading, rcCursor, rcInvalid );
    }

    return VCursorReadColumnDirectInt
        ( self, self -> row_id, col_idx, elem_bits, base, boff, row_len, NULL );
}

static __inline__
bool bad_elem_bits ( uint32_t elem_size, uint32_t elem_bits )
{
    if ( elem_size != elem_bits )
    {
        if ( elem_size < elem_bits && elem_bits % elem_size != 0 )
            return true;
        return ( elem_size % elem_bits != 0 );
    }
    return false;
}

LIB_EXPORT rc_t CC VCursorRead ( const VCursor *self, uint32_t col_idx,
    uint32_t elem_bits, void *buffer, uint32_t blen, uint32_t *row_len )
{
    rc_t rc;

    if ( row_len == NULL )
        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcReading, rcSelf, rcNull );
        else if ( elem_bits == 0 || ( elem_bits & 7 ) != 0 )
            rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcInvalid );
        else
        {
            uint32_t elem_size; const void *base; uint32_t boff;
            rc = VCursorReadColumn ( self, col_idx, & elem_size,
                & base, & boff, row_len );
            if ( rc == 0 )
            {
                if ( bad_elem_bits ( elem_size, elem_bits ) )
                    rc = RC ( rcVDB, rcCursor, rcReading, rcType, rcInconsistent );
                else if ( * row_len != 0 )
                {
                    if ( blen == 0 )
                        return RC ( rcVDB, rcCursor, rcReading, rcBuffer, rcInsufficient );
                    if ( buffer == NULL )
                        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
                    else
                    {
                        uint64_t to_read = * row_len * elem_size;
                        uint64_t bsize = blen * elem_bits;

                        /* always return the required buffer size */
                        * row_len = ( uint32_t ) ( to_read / elem_bits );

                        /* detect buffer too small */
                        if ( to_read > bsize )
                        {
                            rc = RC ( rcVDB, rcCursor, rcReading, rcBuffer, rcInsufficient );
                            to_read = bsize;
                        }

                        /* copy out data up to limit */
                        assert ( boff == 0 );
                        memcpy ( buffer, base, ( size_t ) ( to_read >> 3 ) );

                        return rc;
                    }
                }
            }
        }

        * row_len = 0;
    }

    return rc;
}

LIB_EXPORT rc_t CC VCursorReadDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx,
    uint32_t elem_bits, void *buffer, uint32_t blen, uint32_t *row_len )
{
    rc_t rc;

    if ( row_len == NULL )
        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcReading, rcSelf, rcNull );
        else if ( elem_bits == 0 || ( elem_bits & 7 ) != 0 )
            rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcInvalid );
        else
        {
            uint32_t elem_size; const void *base; uint32_t boff;
            rc = VCursorReadColumnDirect ( self, row_id, col_idx,
                & elem_size, & base, & boff, row_len );
            if ( rc == 0 )
            {
                if ( bad_elem_bits ( elem_size, elem_bits ) )
                    rc = RC ( rcVDB, rcCursor, rcReading, rcType, rcInconsistent );
                else if ( * row_len != 0 )
                {
                    if ( blen == 0 )
                        return RC ( rcVDB, rcCursor, rcReading, rcBuffer, rcInsufficient );
                    if ( buffer == NULL )
                        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
                    else
                    {
                        uint64_t to_read = * row_len * elem_size;
                        uint64_t bsize = blen * elem_bits;

                        /* always return the required buffer size */
                        * row_len = ( uint32_t ) ( to_read / elem_bits );

                        /* detect buffer too small */
                        if ( to_read > bsize )
                        {
                            rc = RC ( rcVDB, rcCursor, rcReading, rcBuffer, rcInsufficient );
                            to_read = bsize;
                        }

                        /* copy out data up to limit */
                        assert ( boff == 0 );
                        memcpy ( buffer, base, ( size_t ) ( to_read >> 3 ) );

                        return rc;
                    }
                }
            }
        }

        * row_len = 0;
    }

    return rc;
}


/* ReadBits
 *  read single row of potentially bit-aligned column data into a buffer
 *
 *  "col_idx" [ IN ] - index of column to be read, returned by "AddColumn"
 *
 *  "elem_bits" [ IN ] - expected element size in bits, required to be
 *  compatible with the actual element size, and may ( or may not ) be
 *  a multiple of 8 ( byte aligned ).
 *
 *  "start" [ IN ] - zero-based starting index to first element,
 *  valid from 0 .. row_len - 1
 *
 *  "buffer" [ IN ], "boff" [ IN ] and "blen" [ IN ] -
 *  return buffer for row data, where "boff" is in BITS
 *  and "blen" is in ELEMENTS.
 *
 *  "num_read" [ OUT ] - return parameter for the number of elements
 *  read, which is <= "blen"
 *
 *  "remaining" [ OUT, NULL OKAY ] - optional return parameter for
 *  the number of elements remaining to be read. specifically,
 *  "start" + "num_read" + "remaining" == row length, assuming that
 *  "start" <= row length.
 */
LIB_EXPORT rc_t CC VCursorReadBits ( const VCursor *self, uint32_t col_idx,
    uint32_t elem_bits, uint32_t start, void *buffer, uint32_t off,
    uint32_t blen, uint32_t *num_read, uint32_t *remaining )
{
    rc_t rc;

    uint32_t dummy;
    if ( remaining == NULL )
        remaining = & dummy;

    if ( num_read == NULL )
        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcReading, rcSelf, rcNull );
        else if ( elem_bits == 0 )
            rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcInvalid );
        else
        {
            uint32_t elem_size; const void *base; uint32_t boff;
            rc = VCursorReadColumn ( self, col_idx, & elem_size,
                & base, & boff, num_read );
            if ( rc == 0 )
            {
                if ( bad_elem_bits ( elem_size, elem_bits ) )
                    rc = RC ( rcVDB, rcCursor, rcReading, rcType, rcInconsistent );
                else if ( * num_read != 0 )
                {
                    uint64_t to_read = * num_read * elem_size;
                    uint64_t doff = start * elem_bits;
                    to_read = to_read > doff ? to_read - doff : 0;
                    if ( blen == 0 )
                    {
                        * num_read = 0;
                        * remaining = ( uint32_t ) ( to_read / elem_bits );
                        return 0;
                    }

                    if ( buffer == NULL )
                        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
                    else
                    {
                        uint64_t bsize = blen * elem_size;
                        if ( to_read <= bsize )
                            * remaining = 0;
                        else
                        {
                            * remaining = (uint32_t)( ( to_read - bsize ) / elem_bits );
                            to_read = bsize;
                        }
                        bitcpy ( buffer, off, base, boff + doff, ( bitsz_t ) to_read );
                        * num_read = ( uint32_t ) ( to_read / elem_bits );
                        return 0;
                    }
                }
            }
        }

        * num_read = 0;
    }

    * remaining = 0;

    return rc;
}

LIB_EXPORT rc_t CC VCursorReadBitsDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx,
    uint32_t elem_bits, uint32_t start, void *buffer, uint32_t off,
    uint32_t blen, uint32_t *num_read, uint32_t *remaining )
{
    rc_t rc;

    uint32_t dummy;
    if ( remaining == NULL )
        remaining = & dummy;

    if ( num_read == NULL )
        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcReading, rcSelf, rcNull );
        else if ( elem_bits == 0 )
            rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcInvalid );
        else
        {
            uint32_t elem_size; const void *base; uint32_t boff;
            rc = VCursorReadColumnDirect ( self, row_id, col_idx,
                & elem_size, & base, & boff, num_read );
            if ( rc == 0 )
            {
                if ( bad_elem_bits ( elem_size, elem_bits ) )
                    rc = RC ( rcVDB, rcCursor, rcReading, rcType, rcInconsistent );
                else if ( * num_read != 0 )
                {
                    uint64_t to_read = * num_read * elem_size;
                    uint64_t doff = start * elem_bits;
                    to_read = to_read > doff ? to_read - doff : 0;
                    if ( blen == 0 )
                    {
                        * num_read = 0;
                        * remaining = ( uint32_t ) ( to_read / elem_bits );
                        return 0;
                    }

                    if ( buffer == NULL )
                        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
                    else
                    {
                        uint64_t bsize = blen * elem_size;
                        if ( to_read <= bsize )
                            * remaining = 0;
                        else
                        {
                            * remaining = (uint32_t)( ( to_read - bsize ) / elem_bits );
                            to_read = bsize;
                        }
                        bitcpy ( buffer, off, base, boff + doff, ( bitsz_t ) to_read );
                        * num_read = ( uint32_t ) ( to_read / elem_bits );
                        return 0;
                    }
                }
            }
        }

        * num_read = 0;
    }

    * remaining = 0;

    return rc;
}


/* CellData
 *  access pointer to single cell of potentially bit-aligned column data
 *
 *  "col_idx" [ IN ] - index of column to be read, returned by "AddColumn"
 *
 *  "elem_bits" [ OUT, NULL OKAY ] - optional return parameter for
 *  element size in bits
 *
 *  "base" [ OUT ] and "boff" [ OUT, NULL OKAY ] -
 *  compound return parameter for pointer to row starting bit
 *  where "boff" is in BITS
 *
 *  "row_len" [ OUT, NULL OKAY ] - the number of elements in row
 */
LIB_EXPORT rc_t CC VCursorCellData ( const VCursor *self, uint32_t col_idx,
    uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len )
{
    rc_t rc;

    uint32_t dummy1, dummy2, dummy3;
    if ( row_len == NULL )
        row_len = & dummy1;
    if ( boff == NULL )
        boff = & dummy2;
    if ( elem_bits == NULL )
        elem_bits = & dummy3;

    if ( base == NULL )
        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcReading, rcSelf, rcNull );
        else
        {
            rc = VCursorReadColumn ( self, col_idx,
                elem_bits, base, boff, row_len );
            if ( rc == 0 )
                return 0;
        }

        * base = NULL;
    }

    * elem_bits = 0;
    * boff = 0;
    * row_len = 0;

    return rc;
}

LIB_EXPORT rc_t CC VCursorCellDataDirect ( const VCursor *self, int64_t row_id, uint32_t col_idx,
    uint32_t *elem_bits, const void **base, uint32_t *boff, uint32_t *row_len )
{
    rc_t rc;

    uint32_t dummy1, dummy2, dummy3;
    if ( row_len == NULL )
        row_len = & dummy1;
    if ( boff == NULL )
        boff = & dummy2;
    if ( elem_bits == NULL )
        elem_bits = & dummy3;

    if ( base == NULL )
        rc = RC ( rcVDB, rcCursor, rcReading, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcReading, rcSelf, rcNull );
        else
        {
            rc = VCursorReadColumnDirect ( self, row_id, col_idx,
                elem_bits, base, boff, row_len );
            if ( rc == 0 )
                return 0;
        }

        * base = NULL;
    }

    * elem_bits = 0;
    * boff = 0;
    * row_len = 0;

    return rc;
}


/* OpenParent
 *  duplicate reference to parent table
 *  NB - returned reference must be released
 */
LIB_EXPORT rc_t CC VCursorOpenParentRead ( const VCursor *self, const VTable **tbl )
{
    rc_t rc;

    if ( tbl == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else
        {
            rc = VTableAddRef ( self -> tbl );
            if ( rc == 0 )
            {
                * tbl = self -> tbl;
                return 0;
            }
        }

        * tbl = NULL;
    }

    return rc;
}


static
void CC insert_overloaded_scolumns ( void *item, void *data )
{
    VCursor *curs = data;
    const SColumn *scol = ( const void* ) item;

    uint32_t ignore;
    VCursorAddSColumn ( curs, & ignore, scol, NULL );
}

static
void VCursorListCol_walk_through_columns_and_add_to_cursor ( VCursor *self )
{
    uint32_t idx = VectorStart ( & self -> stbl -> cname );
    uint32_t end = VectorLength ( & self -> stbl -> cname );
    for ( end += idx; idx < end; ++idx )
    {
        /* look at the table column name guy */
        const SNameOverload* ol_entry = ( const SNameOverload* ) VectorGet ( & self -> stbl -> cname, idx );
        if ( ol_entry != NULL )
            VectorForEach ( & ol_entry -> items, false, insert_overloaded_scolumns, self );
    }
}

static
rc_t VCursorListCol_consolidate_and_insert( const VCursor *self, BSTree *columns )
{
    rc_t rc = VCursorOpenForListing ( self );
    if ( rc == 0 )
    {
        uint32_t idx = VectorStart ( & self -> row );
        uint32_t end = VectorLength ( & self -> row );

        for ( end += idx; idx < end; ++idx )
        {
            const VColumn* vcol = ( const VColumn* ) VectorGet ( & self -> row, idx );
            if ( vcol != NULL )
            {
                VColumnRef *cref;
                rc = VColumnRefMake ( & cref, self -> schema, vcol -> scol );
                if ( rc != 0 )
                    break;

                rc = BSTreeInsert ( columns, & cref -> n, VColumnRefSort );
                assert ( rc == 0 );
            }
        }
    }

    return rc;
}


/* ListReadableColumns
 *  performs an insert of '*' to cursor
 *  attempts to resolve all read rules
 *  records all SColumns that successfully resolved
 *  populates BTree with VColumnRef objects
 */
rc_t VCursorListReadableColumns ( VCursor *self, BSTree *columns )
{
    /* add '*' to cursor */
    VCursorListCol_walk_through_columns_and_add_to_cursor ( self );

    /* insert all columns into tree */
    return VCursorListCol_consolidate_and_insert ( self, columns );
}


/* GetUserData
 *  store/retrieve an opaque pointer to user data
 *
 *  "data" [ OUT ] - return parameter for getting data
 */
LIB_EXPORT rc_t CC VCursorGetUserData ( const VCursor *self, void **data )
{
    rc_t rc;

    if ( data == NULL )
        rc = RC ( rcVDB, rcCursor, rcAccessing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcCursor, rcAccessing, rcSelf, rcNull );
        else
        {
            * data = self -> user;
            return 0;
        }

        * data = NULL;
    }

    return rc;
}

/* SetUserData
 *  store/retrieve an opaque pointer to user data
 *
 *  "data" [ IN ] - parameter for setting data
 *
 *  "destroy" [ IN, NULL OKAY ] - optional destructor param
 *  invoked from destructor of "self"
 */
LIB_EXPORT rc_t CC VCursorSetUserData ( const VCursor *cself,
    void *data, void ( CC * destroy ) ( void *data ) )
{
    VCursor *self = ( VCursor* ) cself;

    if ( self == NULL )
        return RC ( rcVDB, rcCursor, rcUpdating, rcSelf, rcNull );

    self -> user = data;
    self -> user_whack = destroy;

    return 0;
}

LIB_EXPORT rc_t CC VCursorLinkedCursorGet(const VCursor *cself,const char *tbl,VCursor const **curs)
{
    LinkedCursorNode *node;
    VCursor *self = (VCursor *)cself;

    if(cself == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcSelf, rcNull);
    if(tbl == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcNull);
    if(tbl[0] == '\0')
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcEmpty);
    node = (LinkedCursorNode *)BSTreeFind(&self->linked_cursors, tbl, LinkedCursorComp);
    if (node == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcNotFound);

    *curs = node->curs;
    return 0;
}
LIB_EXPORT rc_t CC VCursorLinkedCursorSet(const VCursor *cself,const char *tbl,VCursor const *curs)
{
    LinkedCursorNode *node;
    VCursor *self = (VCursor *)cself;
    rc_t rc;

    if(cself == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcSelf, rcNull);
    if(tbl == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcNull);
    if(tbl[0] == '\0')
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcEmpty);

    node = malloc(sizeof(*node));
    if (node == NULL)
            return RC(rcVDB, rcCursor, rcAccessing, rcMemory, rcExhausted);
    strncpy(node->tbl,tbl,sizeof(node->tbl));
    node->curs=(VCursor*)curs;
    rc = BSTreeInsertUnique(&self->linked_cursors, (BSTNode *)node, NULL, LinkedCursorNodeComp);
    if (rc)
       free(node); 
    else 
	VCursorAddRef(curs);
    return rc;
}


/* private */
LIB_EXPORT rc_t CC VCursorParamsGet( struct VCursorParams const *cself,
    const char *Name, KDataBuffer **value )
{
    NamedParamNode *node;
    String name;
    VCursor *self = (VCursor *)cself;
    
    if (cself == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcSelf, rcNull);
    
    if (Name == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcNull);
    
    if (Name[0] == '\0')
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcEmpty);
    
    StringInitCString(&name, Name);
    node = (NamedParamNode *)BSTreeFind(&self->named_params, &name, NamedParamComp);
    if (node == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcNotFound);
        
    *value = &node->value;
    return 0;
}

/* private */
static rc_t VCursorParamsLookupOrCreate(struct VCursorParams const *cself,
                         const char *Name, KDataBuffer **value)
{
    NamedParamNode *node;
    String name;
    VCursor *self = (VCursor *)cself;
    rc_t rc;
    
    StringInitCString(&name, Name);
    node = (NamedParamNode *)BSTreeFind(&self->named_params, &name, NamedParamComp);
    if (node == NULL) {
        node = malloc(sizeof(*node) + StringSize(&name) + 1);
        if (node == NULL)
            return RC(rcVDB, rcCursor, rcAccessing, rcMemory, rcExhausted);
        
        strcpy((char *)(&node[1]), Name);
        StringInit ( & node -> name, (const char *)(&node[1]), name . size, name . len );
        
        memset ( & node -> value, 0, sizeof node -> value );
        node -> value . elem_bits = 8;
        
        rc = BSTreeInsertUnique(&self->named_params, (BSTNode *)node, NULL, NamedParamNodeComp);
        assert(rc == 0);
    }
    *value = &node->value;
    return 0;
}

LIB_EXPORT rc_t CC VCursorParamsVSet(struct VCursorParams const *cself, 
    const char *Name, const char *fmt, va_list args )
{
    KDataBuffer *value;
    rc_t rc;

    if (cself == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcSelf, rcNull);

    if (Name == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcNull);

    if (Name[0] == '\0')
        return RC(rcVDB, rcCursor, rcAccessing, rcName, rcEmpty);

    rc = VCursorParamsLookupOrCreate(cself, Name, &value);
    if (rc == 0) {
        int n;
        char dummy[1];

        va_list copy;
        va_copy(copy, args);

        /* optimistic printf */
        n = vsnprintf ( ( value -> base == NULL ) ?
            dummy : value -> base, value -> elem_count, fmt, copy );
        va_end(copy);

        if ( n < 0 || n >= (int)value -> elem_count )
        {
            rc = KDataBufferResize ( value, ( n < 0 ) ? 4000 : n + 1 );
            if (rc == 0)
            {
                n = vsnprintf(value->base, value->elem_count, fmt, args);
                if ( n < 0 || n >= (int)value->elem_count )
                {
                    rc = RC ( rcVDB, rcCursor, rcUpdating, rcParam, rcInvalid );
                    KDataBufferWhack ( value );
                }
            }
        }

        if ( rc == 0 )
            value -> elem_count = n;
    }
    return rc;
}

LIB_EXPORT rc_t CC VCursorParamsSet( struct VCursorParams const *cself, 
    const char *name, const char *fmt, ... )
{
    va_list va;
    rc_t rc;

    va_start(va, fmt);
    rc = VCursorParamsVSet(cself, name, fmt, va);
    va_end(va);
    return rc;
}

LIB_EXPORT rc_t CC VCursorParamsUnset( struct VCursorParams const *cself, const char *Name ) {
    KDataBuffer *value;
    rc_t rc;

    if (cself == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcSelf, rcNull);

    if (Name == NULL)
        return RC(rcVDB, rcCursor, rcAccessing, rcParam, rcNull);

    if (Name[0] == '\0')
        return RC(rcVDB, rcCursor, rcAccessing, rcParam, rcInvalid);

    rc = VCursorParamsGet(cself, Name, &value);
    if (rc == 0)
        KDataBufferWhack(value);

    return rc;
}

/*  VCursorGetSchema
 *  returns current schema of the open cursor
 */
LIB_EXPORT struct VSchema const * CC VCursorGetSchema ( struct VCursor const *self )
{
    return self ? self->schema : NULL;
}


static rc_t run_pagemap_thread ( const KThread *t, void *data )
{
    rc_t rc;
    VCursor *self = data;
    /* acquire lock */
    MTCURSOR_DBG (( "run_pagemap_thread: acquiring lock\n" ));
    while((rc = KLockAcquire ( self -> pmpr.lock ))==0){
CHECK_AGAIN:
	switch(self->pmpr.state){
	 case ePMPR_STATE_NONE: 		/* wait for new request */
	 case ePMPR_STATE_SERIALIZE_DONE: 	/* wait for result pickup **/
	 case ePMPR_STATE_DESERIALIZE_DONE:	/* wait for result pickup **/
		MTCURSOR_DBG (( "run_pagemap_thread: waiting for new request\n" ));
		rc = KConditionWait ( self -> pmpr.cond, self -> pmpr.lock );
		goto CHECK_AGAIN;
	 case ePMPR_STATE_EXIT: /** exit requested ***/
		MTCURSOR_DBG (( "run_pagemap_thread: exit by request\n" ));
		KLockUnlock(self -> pmpr.lock);
		return 0;
	 case ePMPR_STATE_DESERIALIZE_REQUESTED:
		MTCURSOR_DBG (( "run_pagemap_thread: request to deserialize\n" ));
		self->pmpr.rc = PageMapDeserialize(&self->pmpr.pm,self->pmpr.data.base,self->pmpr.data.elem_count,self->pmpr.row_count);
		if(self->pmpr.rc == 0){
			self->pmpr.rc=PageMapExpandFull(self->pmpr.pm);
			/*self->pmpr.rc=PageMapExpand(self->pmpr.pm,self->pmpr.row_count<2048?self->pmpr.row_count-1:2048);*/
			assert(self->pmpr.rc == 0);
		}
		self->pmpr.state = ePMPR_STATE_DESERIALIZE_DONE;
		/*fprintf(stderr,"Pagemap %p Done R:%6d|DR:%d|LR:%d\n",self->pmpr.lock, self->pmpr.pm->row_count,self->pmpr.pm->data_recs,self->pmpr.pm->leng_recs);*/
		KLockUnlock(self -> pmpr.lock);
		KConditionSignal ( self -> pmpr.cond );
		break;
	 case ePMPR_STATE_SERIALIZE_REQUESTED:
		MTCURSOR_DBG (( "run_pagemap_thread: request to serialize\n" ));
		self->pmpr.rc = PageMapSerialize(self->pmpr.pm,&self->pmpr.data,0,&self->pmpr.elem_count);
		self->pmpr.state = ePMPR_STATE_SERIALIZE_DONE;
		KLockUnlock(self -> pmpr.lock);
		KConditionSignal ( self -> pmpr.cond );
		break;
	 default:
		assert(0);
		KLockUnlock(self -> pmpr.lock);
		return RC(rcVDB, rcPagemap, rcConverting, rcParam, rcInvalid );
	 
	}
    }
    MTCURSOR_DBG (( "run_pagemap_thread: exit\n" ));
    return rc;
}

rc_t CC VCursorLaunchPagemapThread(VCursor *curs)
{
	rc_t rc;
	curs -> pagemap_thread = NULL; /** if fails - will not use **/
	rc = KLockMake ( & curs -> pmpr.lock );
	if(rc == 0)
		rc = KConditionMake ( & curs -> pmpr.cond );
	if(rc == 0)
		rc = KThreadMake ( & curs -> pagemap_thread, run_pagemap_thread, curs );
	return rc;
}
rc_t CC VCursorTerminatePagemapThread(VCursor *self)
{
	rc_t rc=0;
	if(self -> pagemap_thread != NULL){
		rc = KLockAcquire ( self -> pmpr.lock );
		if ( rc == 0 ){
			self -> pmpr.state = ePMPR_STATE_EXIT;
			KConditionSignal ( self -> pmpr.cond );
			KLockUnlock ( self -> pmpr.lock );
		}
		KThreadWait ( self -> pagemap_thread, NULL );
	}
	KThreadRelease ( self -> pagemap_thread );
	KConditionRelease ( self -> pmpr.cond );
	KLockRelease ( self -> pmpr.lock );
	return rc;
}
