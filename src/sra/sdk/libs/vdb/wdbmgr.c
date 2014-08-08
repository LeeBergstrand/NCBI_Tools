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

#define TRACK_REFERENCES 0
/* should match dbmgr-cmn.c */

#include "libwvdb.vers.h"

#include "dbmgr-priv.h"
#include "schema-priv.h"
#include "linker-priv.h"

#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/vdb-priv.h>
#include <kdb/manager.h>
#include <kfs/directory.h>
#include <kproc/lock.h>
#include <klib/rc.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <assert.h>


/*--------------------------------------------------------------------------
 * VDBManager
 *  opaque handle to library
 */


/* MakeUpdate
 *  create library handle for specific use
 *  NB - only one of the functions will be implemented
 *
 */
LIB_EXPORT rc_t CC VDBManagerMakeUpdate ( VDBManager **mgrp,  KDirectory *wd )
{
    rc_t rc;

    if ( mgrp == NULL )
        rc = RC ( rcVDB, rcMgr, rcConstructing, rcParam, rcNull );
    else
    {
        VDBManager *mgr = malloc ( sizeof * mgr );
        if ( mgr == NULL )
            rc = RC ( rcVDB, rcMgr, rcConstructing, rcMemory, rcExhausted );
        else
        {
            rc = KDBManagerMakeUpdate ( & mgr -> kmgr, wd );
            if ( rc == 0 )
            {
                rc = VSchemaMakeIntrinsic ( & mgr -> schema );
                if ( rc == 0 )
                {
                    rc = VLinkerMakeIntrinsic ( & mgr -> linker );
                    if ( rc == 0 )
                    {
                        rc = VDBManagerConfigPaths ( mgr, true );
                        if ( rc == 0 )
                        {
                            mgr -> user = NULL;
                            mgr -> user_whack = NULL;
                            KRefcountInit ( & mgr -> refcount, 1, "VDBManager", "make-update", "vmgr" );
                            * mgrp = mgr;
                            return 0;
                        }

                        VLinkerRelease ( mgr -> linker );
                    }

                    VSchemaRelease ( mgr -> schema );
                }

                KDBManagerRelease ( mgr -> kmgr );
            }

            free ( mgr );
        }

        * mgrp = NULL;
    }
    return rc;
}


/* Version
 *  returns the library version
 */
LIB_EXPORT rc_t CC VDBManagerVersion ( const VDBManager *self, uint32_t *version )
{
    if ( version == NULL )
        return RC ( rcVDB, rcMgr, rcAccessing, rcParam, rcNull );
    if ( self == NULL )
        return RC ( rcVDB, rcMgr, rcAccessing, rcSelf, rcNull );

    * version = LIBWVDB_VERS;
    return 0;
}

/* Lock
 *  apply lock
 *
 *  if object is already locked, the operation is idempotent
 *  and returns an rc state of rcLocked
 *
 *  "path" [ IN ] - NUL terminated path
 */
LIB_EXPORT rc_t CC VDBManagerVLock ( VDBManager *self, const char *path, va_list args )
{
    if ( self == NULL )
        return RC ( rcVDB, rcMgr, rcLocking, rcSelf, rcNull );
    return KDBManagerVLock ( self -> kmgr, path, args );
}

LIB_EXPORT rc_t CC VDBManagerLock ( VDBManager *self, const char *path, ... )
{
    rc_t rc;

    va_list args;
    va_start ( args, path );

    rc = VDBManagerVLock ( self, path, args );

    va_end ( args );

    return rc;
}

/* Unlock
 *  remove lock
 *
 *  if object is already unlocked, the operation is idempotent
 *  and returns an rc state of rcUnlocked
 *
 *  "path" [ IN ] - NUL terminated path
 */
LIB_EXPORT rc_t CC VDBManagerVUnlock ( VDBManager *self, const char *path, va_list args )
{
    if ( self == NULL )
        return RC ( rcVDB, rcMgr, rcUnlocking, rcSelf, rcNull );
    return KDBManagerVUnlock ( self -> kmgr, path, args );
}

LIB_EXPORT rc_t CC VDBManagerUnlock ( VDBManager *self, const char *path, ... )
{
    rc_t rc;

    va_list args;
    va_start ( args, path );

    rc = VDBManagerVUnlock ( self, path, args );

    va_end ( args );

    return rc;
}


/* Drop
 *  drop an object based on its path
 *
 *  "path" [ IN ] - NUL terminated string in UTF-8 giving path to the vdb object
 */
LIB_EXPORT rc_t CC VDBManagerVDrop ( VDBManager *self, uint32_t obj_type, const char *path, va_list args )
{
    if ( self != NULL )
        return KDBManagerVDrop ( self -> kmgr, obj_type, path, args );

    return RC ( rcVDB, rcMgr, rcRemoving, rcSelf, rcNull );
}

LIB_EXPORT rc_t CC VDBManagerDrop ( VDBManager *self, uint32_t obj_type, const char *path, ... )
{
    rc_t rc;

    va_list args;
    va_start ( args, path );

    rc = VDBManagerVDrop ( self, obj_type, path, args );

    va_end ( args );

    return rc;
}


/* VDBManagerRunPeriodicTasks
 *  executes periodic tasks, such as cache flushing
 */
LIB_EXPORT rc_t CC VDBManagerRunPeriodicTasks ( const VDBManager *self )
{
    if ( self == NULL )
        return RC ( rcVDB, rcMgr, rcExecuting, rcSelf, rcNull );

    return KDBManagerRunPeriodicTasks ( self -> kmgr );
}

/* OpenKDBManager
 *  returns a new reference to KDBManager used by VDBManager
 */
LIB_EXPORT rc_t CC VDBManagerOpenKDBManagerUpdate ( VDBManager *self, KDBManager **kmgr )
{
    rc_t rc;

    if ( kmgr == NULL )
        rc = RC ( rcVDB, rcMgr, rcAccessing, rcParam, rcNull );
    else
    {
        if ( self == NULL )
            rc = RC ( rcVDB, rcMgr, rcAccessing, rcSelf, rcNull );
        else
        {
            rc = KDBManagerAddRef ( self -> kmgr );
            if ( rc == 0 )
            {
                * kmgr = self -> kmgr;
                return 0;
            }
        }

        * kmgr = NULL;
    }

    return rc;
}
