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

#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/table.h>
#include <vdb/cursor.h>

#include <kapp/main.h>
#include <kapp/args.h>
#include <klib/text.h>
#include <klib/log.h>
#include <klib/rc.h>

/* #include <fmtdef.h> */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static const char *table_path = "static_test";
static const char buff[] = "this is a test row for static data\n";

static rc_t static_write(void)
{
    const char *schema_text = "version 1; include 'vdb/vdb.vschema'; table foo #1 { column ascii bar { read = .bar; } physical ascii .bar = bar; }";
    bool force = 1;
    rc_t rc;
    VDBManager *vmgr;
    
    rc = VDBManagerMakeUpdate ( & vmgr, NULL );
    if ( rc != 0 )
        LOGERR ( klogInt, rc, "failed to make VDB manager" );
    else
    {
        VSchema *schema;
        rc = VDBManagerMakeSchema ( vmgr, & schema );
        if ( rc != 0 )
            LOGERR ( klogInt, rc, "failed to make empty schema" );
        else
        {
            rc = VSchemaParseText( schema, "static_schema", schema_text, strlen(schema_text) );
            if ( rc != 0 )
                PLOGERR ( klogErr, (klogErr, rc, "failed to parse schema '$(text)'", "test=%s", schema_text ));
            else
            {
                VTable *vtbl;
                
                rc = VDBManagerCreateTable ( vmgr, & vtbl, schema, "foo", force ? kcmInit : kcmCreate, table_path );
                if ( rc != 0 )
                {
                    PLOGERR ( klogErr, (klogErr, rc, "failed to $(cmode) table '$(path)'"
                             , "cmode=%s,path=%s"
                             , force ? "create or replace" : "create"
                             , table_path
                                        ));
                }
                else
                {
                    VCursor *curs;
                    rc = VTableCreateCursorWrite ( vtbl, & curs, kcmInsert );
                    if ( rc != 0 )
                        LOGERR ( klogInt, rc, "failed to create cursor" );
                    else
                    {
                        uint32_t idx;
                        
                        rc = VCursorAddColumn ( curs, &idx, "bar" );
                        if ( rc != 0 )
                        {
                            PLOGERR ( klogErr, (klogErr, rc, "failed to add column '$(col)' to cursor"
                                     , "col=bar"
                                     ));
                        }
                        else {
                            rc = VCursorOpen ( curs );
                            if ( rc != 0 )
                                LOGERR ( klogErr, rc, "failed to open cursor" );
                            else
                            {
                                int i;
                                
                                for ( i = 0; i != 10 && rc == 0; ++i )
                                {
                                    rc = VCursorOpenRow ( curs );
                                    if ( rc != 0 )
                                        LOGERR ( klogErr, rc, "failed to open cursor row" );
                                    else
                                    {
                                        rc_t rc2;
                                        uint32_t count = sizeof(buff) - 1;
                                        
                                        rc = VCursorWrite ( curs, idx, 8, buff, 0, count );
                                        if ( rc != 0 )
                                        {
                                            int64_t rid = 0;
                                            VCursorRowId ( curs, & rid );
                                            
                                            PLOGERR ( klogInt, (klogInt, rc, "failed to write data to row $(row_id)'"
                                                     , "row_id=%ld"
                                                     , rid
                                                     ));
                                            break;
                                        }
                                        if ( rc == 0 )
                                        {
                                            rc = VCursorCommitRow ( curs );
                                            if ( rc != 0 )
                                                LOGERR ( klogErr, rc, "failed to commit row" );
                                        }
                                        
                                        rc2 = VCursorCloseRow ( curs );
                                        if ( rc2 != 0 )
                                        {
                                            LOGERR ( klogErr, rc2, "failed to close cursor row" );
                                            if ( rc == 0 )
                                                rc = rc2;
                                        }
                                    }
                                }
                                
                                if ( GetRCState ( rc ) == rcDone )
                                    rc = 0;
                                
                                if ( rc == 0 )
                                    rc = VCursorCommit ( curs );
                            }
                        }
                        
                        VCursorRelease ( curs );
                    }
                    
#if 1
                    if ( rc == 0 )
                        rc = VTableReindex ( vtbl );
#endif
                    
                    VTableRelease ( vtbl );
                }
            }
            
            VSchemaRelease ( schema );
        }
        
        VDBManagerRelease ( vmgr );
    }
    
    return rc;
}

static rc_t static_read(void) {
    rc_t rc;
    VDBManager *vmgr;
    
    rc = VDBManagerMakeUpdate ( & vmgr, 0 );
    if ( rc != 0 )
        LOGERR ( klogInt, rc, "failed to make VDB manager" );
    else
    {
        const VTable *vtbl;
        
        rc = VDBManagerOpenTableRead(vmgr, &vtbl, 0, table_path );
        if ( rc != 0 )
        {
            PLOGERR ( klogErr, (klogErr, rc, "failed to $(cmode) table '$(path)'"
                     , "cmode=%s,path=%s"
                     , "open"
                     , table_path
                     ));
        }
        else
        {
            const VCursor *curs;
            
            rc = VTableCreateCursorRead(vtbl, &curs);
            if (rc == 0) {
                uint32_t idx;
                
                rc = VCursorAddColumn(curs, &idx, "bar");
                if (rc == 0) {
                    rc = VCursorOpen(curs);
                    if (rc == 0) {
                        int i;
                        
                        for (i = 0; rc == 0 && i != 10; ++i) {
                            rc = VCursorOpenRow(curs);
                            
                            if (rc == 0) {
                                char buffer[1024];
                                uint32_t len;
                                
                                rc = VCursorRead(curs, idx, 8, buffer, sizeof(buffer), &len);
                                assert(len == strlen(buff));
                                assert(memcmp(buff, buffer, len) == 0);
                            }
                            VCursorCloseRow(curs);
                        }
                    }
                    else {
                        PLOGERR ( klogErr, (klogErr, rc, "failed to $(cmode) cursor"
                                 , "cmode=open"
                                 ));
                    }

                }
                else {
                    PLOGERR ( klogErr, (klogErr, rc, "failed to add column $(col)"
                             , "col=bar"
                             ));
                }

                VCursorRelease(curs);
            }
            else {
                PLOGERR ( klogErr, (klogErr, rc, "failed to $(cmode) cursor"
                         , "cmode=create"
                         ));
            }
            VTableRelease(vtbl);
        }
        VDBManagerRelease(vmgr);
    }
    return rc;
}

/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return 0;
}

/* Usage - EXTERN
 *  comments?
 */
const char UsageDefaultName[] = "test-static";
rc_t CC UsageSummary (const char * progname)
{
    return 0;
}
rc_t CC Usage ( const Args * args )
{
    return 0;
}

    
rc_t CC KMain ( int argc, char *argv [] )
{
    bool failed;

    failed =  static_write();

    if (failed)
    {
        printf("static_write failed\n");
    }
    else
    {
        printf("static_write passed\n");
#if 1
        failed = static_read();

        if (failed)
        {
            printf("static_read failed\n");
        }
        else
        {
            printf("static_read passed\n");
        }
#endif
    }
    if (failed)
        printf("static-test failed\n");
    else
        printf("test-static passed\n");

    return 1;
}
