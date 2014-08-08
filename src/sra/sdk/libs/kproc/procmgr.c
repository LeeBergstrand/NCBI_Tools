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

#include <kproc/extern.h>

#include <kproc/procmgr.h>
#include <kproc/task.h>
#include <kproc/lock.h>
#include <klib/refcount.h>
#include <klib/rc.h>

#define rcTask rcCmd

#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*--------------------------------------------------------------------------
 * KCleanupTaskQueue
 */
typedef struct KCleanupTaskQueue KCleanupTaskQueue;
struct KCleanupTaskQueue
{
    uint64_t count;
    uint64_t capacity;
    uint64_t start;
    KTask *q [ 1 ];
};


/*--------------------------------------------------------------------------
 * KProcMgr
 */
struct KProcMgr
{
    KLock *cleanup_lock;
    KCleanupTaskQueue *cleanup;
    KRefcount refcount;
};

static KProcMgr * s_proc_mgr;


/* Whack
 *  tear down proc mgr
 *  runs any outstanding cleanup tasks
 *  deletes the singleton object
 *  intended to be called from an "atexit()" or similar task
 */
LIB_EXPORT rc_t CC KProcMgrWhack ( void )
{
    rc_t rc = 0;

    KProcMgr *self = s_proc_mgr;
    if ( s_proc_mgr != NULL )
    {
        s_proc_mgr = NULL;

        rc = KLockAcquire ( self -> cleanup_lock );
        if ( rc == 0 )
        {
            uint64_t i;

            KCleanupTaskQueue *cleanup = self -> cleanup;
            self -> cleanup = NULL;
            KLockUnlock ( self -> cleanup_lock );

            if ( cleanup != NULL )
            {
                for ( i = 0; i < cleanup -> count; ++ i )
                {
                    KTask *task = cleanup -> q [ i ];
                    if ( task != NULL )
                    {
                        rc_t task_rc = KTaskExecute ( task );
                        if ( rc == 0 )
                            rc = task_rc;

                        cleanup -> q [ i ] = NULL;
                        KTaskRelease ( task );
                    }
                }

                free ( cleanup );
            }
        }

        KLockRelease ( self -> cleanup_lock );
        free ( self );
    }

    return rc;
}


/* Init
 *  initialize the proc mgr
 *  creates the singleton object
 */
LIB_EXPORT rc_t CC KProcMgrInit ( void )
{
    rc_t rc = 0;

    if ( s_proc_mgr == NULL )
    {
        KProcMgr *mgr = malloc ( sizeof * s_proc_mgr );
        if ( mgr == NULL )
            rc = RC ( rcPS, rcMgr, rcInitializing, rcMemory, rcExhausted );
        else
        {
            rc = KLockMake ( & mgr -> cleanup_lock );
            if ( rc == 0 )
            {
                mgr -> cleanup = NULL;
                KRefcountInit ( & mgr -> refcount, 0, "KProcMgr", "init", "process mgr" );

                s_proc_mgr = mgr;
                return 0;
            }

            free ( mgr );
        }
    }

    return rc;
}


/* MakeSingleton
 *  make or access singleton process manager
 *  VDB-2 ONLY
 */
LIB_EXPORT rc_t CC KProcMgrMakeSingleton ( KProcMgr ** mgrp )
{
    rc_t rc;

    if ( mgrp == NULL )
        rc = RC ( rcPS, rcMgr, rcConstructing, rcParam, rcNull );
    else
    {
        * mgrp = s_proc_mgr;

        if ( s_proc_mgr == NULL )
            rc = RC ( rcPS, rcMgr, rcConstructing, rcMgr, rcNull );
        else
            rc = KProcMgrAddRef ( s_proc_mgr );
    }

    return rc;
}


/* AddRef
 * Release
 */
LIB_EXPORT rc_t CC KProcMgrAddRef ( const KProcMgr *self )
{
    if ( self != NULL )
    {
        switch ( KRefcountAdd ( & self -> refcount, "KProcMgr" ) )
        {
        case krefLimit:
            return RC ( rcPS, rcMgr, rcAttaching, rcRange, rcExcessive );
        }
    }
    return 0;
}

LIB_EXPORT rc_t CC KProcMgrRelease ( const KProcMgr *self )
{
    if ( self != NULL )
    {
        switch ( KRefcountDrop ( & self -> refcount, "KProcMgr" ) )
        {
        case krefWhack:
            return 0;
        case krefLimit:
            return RC ( rcPS, rcMgr, rcReleasing, rcRange, rcExcessive );
        }
    }
    return 0;
}


/* AddCleanupTask
 *  add a task to be performed at process exit time
 *
 *  "ticket" [ OUT ] - an id that can be used later to remove task
 *
 *  "task" [ IN ] - task object that will be executed at process
 *   exit, unless previously removed ( see RemoveCleanupTask ).
 *   NB - a new reference to "task" will be created
 */
LIB_EXPORT rc_t CC KProcMgrAddCleanupTask ( KProcMgr *self, KTaskTicket *ticket, KTask *task )
{
    rc_t rc;

    if ( ticket == NULL )
        rc = RC ( rcPS, rcQueue, rcInserting, rcId, rcNull );
    else
    {
        memset ( ticket, 0, sizeof * ticket );

        if ( self == NULL )
            rc = RC ( rcPS, rcQueue, rcInserting, rcSelf, rcNull );
        else if ( self != s_proc_mgr )
            rc = RC ( rcPS, rcQueue, rcInserting, rcSelf, rcIncorrect );
        else if ( task == NULL )
            rc = RC ( rcPS, rcQueue, rcInserting, rcTask, rcNull );
        else
        {
            rc = KTaskAddRef ( task );
            if ( rc == 0 )
            {
                rc = KLockAcquire ( self -> cleanup_lock );
                if ( rc == 0 )
                {
                    const uint64_t extend = 1024;
                    KCleanupTaskQueue *cleanup = self -> cleanup;

                    /* on first insert, allocate queue */
                    if ( self -> cleanup == NULL )
                    {
                        cleanup = malloc ( sizeof * cleanup - sizeof cleanup -> q
                            + sizeof cleanup -> q [ 0 ] * extend );
                        if ( cleanup == NULL )
                            rc = RC ( rcPS, rcQueue, rcInserting, rcMemory, rcExhausted );
                        else
                        {
                            cleanup -> count = 0;
                            cleanup -> capacity = extend;
                            cleanup -> start = 0;

                            self -> cleanup = cleanup;
                        }
                    }

                    /* if queue is full */
                    else if ( cleanup -> count == cleanup -> capacity )
                    {
                        uint64_t empty;

                        /* determine number of empty slots at start */
                        for ( empty = 0; empty < cleanup -> count; ++ empty )
                        {
                            if ( cleanup -> q [ empty ] != NULL )
                                break;
                        }

                        /* if none are empty, simply reallocate - probably bad... */
                        if ( empty == 0 )
                        {
                            cleanup = realloc ( cleanup, sizeof * cleanup - sizeof cleanup -> q
                                + sizeof cleanup -> q [ 0 ] * ( cleanup -> capacity + extend ) );
                            if ( cleanup == NULL )
                                rc = RC ( rcPS, rcQueue, rcInserting, rcMemory, rcExhausted );
                            else
                            {
                                cleanup -> capacity += extend;
                                self -> cleanup = cleanup;
                            }
                        }

                        /* otherwise, rewrite to remove empty slots */
                        else
                        {
                            uint64_t i;
                            const KCleanupTaskQueue *orig = cleanup;

                            /* only bother with resize if there are enough to recover */
                            if ( empty >= 4096 )
                            {
                                cleanup = malloc ( sizeof * cleanup - sizeof cleanup -> q
                                    + sizeof cleanup -> q [ 0 ] * ( orig -> capacity - empty + extend ) );
                                if ( cleanup == NULL )
                                    rc = RC ( rcPS, rcQueue, rcInserting, rcMemory, rcExhausted );
                                else
                                {
                                    cleanup -> capacity = orig -> capacity - empty + extend;
                                }
                            }

                            /* rewrite count to remove leading NULL entries */
                            cleanup -> count = orig -> count - empty;

                            /* accumulate NULL entries into the start offset */
                            cleanup -> start = orig -> start + empty;

                            /* copy the tasks */
                            for ( i = 0; i < cleanup -> count; ++ i )
                                cleanup -> q [ i ] = orig -> q [ empty + i ];

                            if ( ( const KCleanupTaskQueue* ) cleanup != orig )
                            {
                                self -> cleanup = cleanup;
                                free ( ( void* ) orig );
                            }
                        }
                    }

                    /* detect valid queue for insertion */
                    if ( rc == 0 )
                    {
                        /* assert that there is space for a task */
                        assert ( cleanup -> count < cleanup -> capacity );

                        /* record reference to task */
                        cleanup -> q [ cleanup -> count ] = task;

                        /* fill out the ticket information */
                        ticket -> info [ 0 ] =  cleanup -> start + cleanup -> count;
                        ticket -> info [ 1 ] = ( size_t ) task;
                        ticket -> info [ 0 ] ^= ( size_t ) self;
                        ticket -> info [ 1 ] ^= ( size_t ) self;
                        ticket -> info [ 0 ] ^= ( size_t ) task;

                        /* account for insertion */
                        ++ cleanup -> count;
                    }

                    KLockUnlock ( self -> cleanup_lock );
                }

                if ( rc != 0 )
                    KTaskRelease ( task );
            }
        }
    }

    return rc;
}


/* RemoveCleanupTask
 *  remove a task from exit queue
 *  releases reference to task object
 *  returns an rcNotFound code if task no longer exists
 */
LIB_EXPORT rc_t CC KProcMgrRemoveCleanupTask ( KProcMgr *self, const KTaskTicket *ticket )
{
    rc_t rc;

    if ( self == NULL )
        rc = RC ( rcPS, rcQueue, rcRemoving, rcSelf, rcNull );
    else if ( self != s_proc_mgr )
        rc = RC ( rcPS, rcQueue, rcRemoving, rcSelf, rcIncorrect );
    else if ( ticket == NULL )
        rc = RC ( rcPS, rcQueue, rcRemoving, rcId, rcNull );
    else
    {
        /* recover task* and idx */
        KTask *task = ( KTask* ) ( ticket -> info [ 1 ] ^ ( size_t ) self );
        uint64_t idx = ticket -> info [ 0 ] ^ ( size_t ) task;
        idx ^= ( size_t ) self;

        /* go into queue */
        rc = KLockAcquire ( self -> cleanup_lock );
        if ( rc == 0 )
        {
            KCleanupTaskQueue *cleanup = self -> cleanup;
            if ( self -> cleanup == NULL )
                rc = RC ( rcPS, rcQueue, rcRemoving, rcItem, rcNotFound );
            else if ( idx < cleanup -> start )
                rc = RC ( rcPS, rcQueue, rcRemoving, rcItem, rcNotFound );
            else
            {
                idx -= cleanup -> start;
                if ( idx >= cleanup -> count )
                    rc = RC ( rcPS, rcQueue, rcRemoving, rcItem, rcNotFound );
                else if ( cleanup -> q [ idx ] != task )
                    rc = RC ( rcPS, rcQueue, rcRemoving, rcItem, rcNotFound );
                else
                {
                    cleanup -> q [ idx ] = NULL;
                }
            }

            KLockUnlock ( self -> cleanup_lock );
        }

        if ( rc == 0 )
            KTaskRelease ( task );
    }

    return rc;
}
