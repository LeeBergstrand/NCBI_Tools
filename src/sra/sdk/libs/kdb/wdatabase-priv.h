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

#ifndef _h_database_priv_
#define _h_database_priv_

#ifndef _h_kdb_manager_
#include <kdb/manager.h>
#endif

#ifndef _h_kdb_database_
#include <kdb/database.h>
#endif

#ifndef _h_klib_container_
#include <klib/container.h>
#endif

#ifndef _h_klib_refcount_
#include <klib/refcount.h>
#endif

#include <klib/symbol.h>
#include <kfs/md5.h>

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * KDatabase
 *  connection to a database within file system
 */
struct KDatabase
{
    /* manager reference */
    KDBManager *mgr;

    /* if a sub-directory */
    KDatabase *dad;

    /* database directory */
    KDirectory *dir;

    KMD5SumFmt *md5;

    /* open references */
    KRefcount refcount;
    uint32_t opencount;
    bool use_md5;
    bool read_only;

    KSymbol sym;

    char path [1];
};

/* Attach
 * Sever
 *  like AddRef/Release, except called internally
 *  indicates that a child object is letting go...
 */
KDatabase *KDatabaseAttach ( const KDatabase *self );
rc_t KDatabaseSever ( const KDatabase *self );


#ifdef __cplusplus
}
#endif

#endif /* _h_database_priv_ */
