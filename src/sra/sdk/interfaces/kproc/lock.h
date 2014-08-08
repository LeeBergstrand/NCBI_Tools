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

#ifndef _h_kproc_lock_
#define _h_kproc_lock_

#ifndef _h_kproc_extern_
#include <kproc/extern.h>
#endif

#ifndef _h_klib_defs_
#include <klib/defs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * forwards
 */
struct timeout_t;


/*--------------------------------------------------------------------------
 * KLock
 *  a POSIX-style mutual exclusion lock
 */
typedef struct KLock KLock;


/* Make
 *  make a simple mutex
 */
KPROC_EXTERN rc_t CC KLockMake ( KLock **lock );


/* AddRef
 * Release
 */
KPROC_EXTERN rc_t CC KLockAddRef ( const KLock *self );
KPROC_EXTERN rc_t CC KLockRelease ( const KLock *self );


/* Acquire
 *  acquires lock
 */
KPROC_EXTERN rc_t CC KLockAcquire ( KLock *self );
KPROC_EXTERN rc_t CC KLockTimedAcquire ( KLock *self, struct timeout_t *tm );

/* Unlock
 *  releases lock
 */
KPROC_EXTERN rc_t CC KLockUnlock ( KLock *self );


/*--------------------------------------------------------------------------
 * KRWLock
 *  a POSIX-style read/write lock
 */
typedef struct KRWLock KRWLock;


/* Make
 *  make a simple read/write lock
 */
KPROC_EXTERN rc_t CC KRWLockMake ( KRWLock **lock );


/* AddRef
 * Release
 */
KPROC_EXTERN rc_t CC KRWLockAddRef ( const KRWLock *self );
KPROC_EXTERN rc_t CC KRWLockRelease ( const KRWLock *self );


/* AcquireShared
 *  acquires read ( shared ) lock
 */
KPROC_EXTERN rc_t CC KRWLockAcquireShared ( KRWLock *self );
KPROC_EXTERN rc_t CC KRWLockTimedAcquireShared ( KRWLock *self, struct timeout_t *tm );


/* AcquireExcl
 *  acquires write ( exclusive ) lock
 */
KPROC_EXTERN rc_t CC KRWLockAcquireExcl ( KRWLock *self );
KPROC_EXTERN rc_t CC KRWLockTimedAcquireExcl ( KRWLock *self, struct timeout_t *tm );


/* Unlock
 *  releases lock
 */
KPROC_EXTERN rc_t CC KRWLockUnlock ( KRWLock *self );


#ifdef __cplusplus
}
#endif

#endif /* _h_kproc_lock_ */
