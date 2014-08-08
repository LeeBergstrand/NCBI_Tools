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

/* #include "vdb-test-ab.vers.h" */

#include <vdb/manager.h>
#include <vdb/table.h>
#include <vdb/cursor.h>

#include <kapp/main.h>
#include <klib/log.h>
#include <klib/rc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static
rc_t dump_cursor ( const VCursor *curs, uint32_t idx )
{
    rc_t rc;
    uint32_t row_num;

    for ( rc = 0, row_num = 0; rc == 0; ++row_num)
    {
        rc = VCursorOpenRow ( curs );
        if ( rc == 0 )
        {
            rc_t rc2;

            uint32_t row_len;
            char buffer [ 1024 ];
            rc = VCursorRead ( curs, idx, 8, buffer,
                ( uint32_t ) sizeof buffer, & row_len );
            if ( rc == 0 )
                fwrite ( buffer, 1, row_len, stdout );

            rc2 = VCursorCloseRow ( curs );
            if ( rc == 0 )
                rc = rc2;
        }
    }

    if ( rc != 0 && !(GetRCState(rc) == rcNotFound && GetRCObject(rc) == rcRow) )
        pLOGERR ( klogErr, rc, "dump_cursor: $(row) rows", "row=%u", row_num );
    else
    {
        PLOGMSG ( klogInfo, "dump_cursor: finished, dump $(row) rows", "row=%u", row_num );
        rc = 0;
    }

    return rc;
}

/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
uint32_t KAppVersion ( void )
{
    return 0x01000000;
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
rc_t KMain ( int argc, char *argv [] )
{
    const VDBManager *vmgr;
    rc_t rc = VDBManagerMakeRead ( & vmgr, NULL );
    if ( rc == 0 )
    {
        const VTable *vtbl;
        rc = VDBManagerOpenTableRead ( vmgr, & vtbl, NULL, argv [ 1 ] );
        if ( rc == 0 )
        {
            const VCursor *curs;
            rc = VTableCreateCursorRead ( vtbl, & curs );
            if ( rc == 0 )
            {
                /* need to be able to add '*' in future */
                uint32_t idx;
                rc = VCursorAddColumn ( curs, & idx, "col1" );
                if ( rc == 0 )
                {
                    rc = VCursorOpen ( curs );
                    if ( rc == 0 )
                        rc = dump_cursor ( curs, idx );
                }

                VCursorRelease ( curs );
            }

            VTableRelease ( vtbl );
        }

        VDBManagerRelease ( vmgr );
    }
    return rc;
}
