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

#include "vdb-test-ac.vers.h"

#include <vdb/manager.h>
#include <vdb/database.h>
#include <vdb/schema.h>
#include <vdb/table.h>

#include <kapp/main.h>
#include <klib/log.h>
#include <klib/rc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
uint32_t KAppVersion ( void )
{
    return VDB_TEST_AC_VERS;
}
    
/* KMain - EXTERN
 *  executable entrypoint "main" is implemented by
 *  an OS-specific wrapper that takes care of establishing
 *  signal handlers, logging, etc.
 *
 *  in turn, OS-specific "main" will invoke "KMain" as
 *  platform independent main entrypoint.
 *
 *  "argc" [ IN ] - the number of textual parameters in "argv"
 *  should never be < 0, but has been left as a signed int
 *  for reasons of tradition.
 *
 *  "argv" [ IN ] - array of NUL terminated strings expected
 *  to be in the shell-native character set: ASCII or UTF-8
 *  element 0 is expected to be executable identity or path.
 */
#if _DEBUGGING
void VSchemaListSymtab ( const VSchema *self );
#endif

rc_t KMain ( int argc, char *argv [] )
{
    VDBManager *vmgr;
    rc_t rc = VDBManagerMakeUpdate ( & vmgr, NULL );
    if ( rc != 0 )
        LOGERR ( klogInt, rc, "failed to make update manager" );
    else
    {
        VSchema *vschema;
        rc = VDBManagerMakeSchema ( vmgr, & vschema );
        if ( rc != 0 )
            LOGERR ( klogInt, rc, "failed to make empty schema" );
        else
        {
            const char *file = "align/align.vschema";
#if _DEBUGGING
            VSchemaListSymtab ( vschema );
#endif
            rc = VSchemaParseFile ( vschema, "align/align.vschema" );
#if _DEBUGGING
            VSchemaListSymtab ( vschema );
#endif
            if ( rc != 0 )
            {
                pLOGERR ( klogErr, rc, "failed to parse schema file '$(file)'"
                          , "file=%s"
                          , file );
            }
            else
            {
                VDatabase *vdb;
                const char *path = "test-ac-db";
                const char *typespec = "NCBI:align:db:alignment";
                rc = VDBManagerCreateDB ( vmgr, & vdb, vschema, typespec, kcmInit, path );
                if ( rc != 0 )
                {
                    pLOGERR ( klogInt, rc, "failed to create '$(typespec)' db at '$(path)'"
                              , "typespec=%s,path=%s"
                              , typespec
                              , path
                        );
                }
                else
                {
                    VTable *vtbl;
                    const char *member = "align";
                    path = member;
                    rc = VDatabaseCreateTable ( vdb, & vtbl, member, kcmInit, path );
                    if ( rc != 0 )
                    {
                        pLOGERR ( klogInt, rc, "failed to create '$(member)' table at '$(path)'"
                                  , "member=%s,path=%s"
                                  , member
                                  , path
                            );
                    }
                    else
                    {
                        printf ( "done\n" );

                        VTableRelease ( vtbl );
                    }

                    VDatabaseRelease ( vdb );
                }
            }

            VSchemaRelease ( vschema );
        }

        VDBManagerRelease ( vmgr );
    }
    return rc;
}
