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

#include "mini-read.vers.h"

#include <vdb/manager.h>
#include <vdb/schema.h>
#include <vdb/table.h>
#include <vdb/cursor.h>

#include <kapp/main.h>
#include <klib/text.h>
#include <klib/log.h>
#include <klib/rc.h>

#include <fmtdef.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

static rc_t mini_read(const char *table_path, const char *col_name, int64_t start, int64_t rowcount) {
    rc_t rc;
    const VDBManager *vmgr;
    
    rc = VDBManagerMakeRead ( & vmgr, 0 );
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
                
                rc = VCursorAddColumn(curs, &idx, col_name);
                if (rc == 0) {
                    VTypedecl type;
                    VTypedesc desc;
                    
                    rc = VCursorDatatype(curs, idx, &type, &desc);
                    if (rc == 0) {
                        uint32_t num_bits;
                        
                        num_bits = VTypedescSizeof(&desc);
                        rc = VCursorOpen(curs);
                        if (rc == 0) {
                            if (start > 0)
                                rc = VCursorSetRowId(curs, start);
                            if (rc == 0) {
                                uint64_t row = 0;
                                
                                do {
                                    if (rowcount >= 0 && row >= rowcount)
                                        break;
                                    rc = VCursorOpenRow(curs);
                                    if (rc == 0) {
                                        uint32_t num_read;
                                        uint32_t remaining;
                                        
                                        rc = VCursorReadBits(curs, idx, num_bits, 0, 0, 0, 0, &num_read, &remaining);
                                        if (rc == 0) {
                                            rc = VCursorCloseRow(curs);
                                        }
                                        else if (rowcount > 0) {
                                            PLOGERR ( klogErr, (klogErr, rc, "failed to read row $(row)"
                                                     , "row=%u", (unsigned int)row + 1
                                                     ));
                                        }
                                    }
                                    else {
                                        PLOGERR ( klogErr, (klogErr, rc, "failed to open row $(row)"
                                                 , "row=%u", (unsigned int)row + 1
                                                 ));
                                    }
                                    ++row;
                                } while (rc == 0);
                            }
                        }
                        else {
                            PLOGERR ( klogErr, (klogErr, rc, "failed to $(cmode) cursor"
                                     , "cmode=open"
                                     ));
                        }
                    }
                    else {
                        PLOGERR ( klogErr, (klogErr, rc, "failed to get datatype for column $(col)"
                                 , "col=%s", col_name
                                 ));
                    }
                }
                else {
                    PLOGERR ( klogErr, (klogErr, rc, "failed to add column $(col)"
                             , "col=%s", col_name
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
uint32_t KAppVersion ( void )
{
    /* please don't subvert versioning... */
    return MINI_READ_VERS;
}

static int my_from_string(const char *str, int64_t *y) {
    unsigned long x;
    char *endp;
    
    x = strtoul(str, &endp, 0);
    if (*endp != '\0')
        return 1;
    *y = x;
    return 0;
}

static const char *prog;

rc_t KMain ( int argc, char *argv [] )
{
    char *table_path = 0;
    char *col_name;
    int64_t start = -1;
    int64_t rowcount = -1;

    prog = *argv;
    col_name = strrchr(prog, '/');
    if (col_name) {
        prog = col_name + 1;
        col_name = 0;
    }
    while (++argv, --argc) {
        if (argv[0][0] == '-') {
            switch (argv[0][1]) {
            case 'i':
                if (table_path)
                    goto USAGE;
                table_path = &argv[0][2];
                if (table_path[0] == '\0') {
                    table_path = argv[1];
                    ++argv, --argc;
                }
                break;
            case 's':
                if (start != -1)
                    goto USAGE;
                col_name = &argv[0][2];
                if (col_name[0] == '\0') {
                    col_name = argv[1];
                    ++argv, --argc;
                }
                if (my_from_string(col_name, &start))
                    goto USAGE;
                col_name = 0;
                break;
            case 'n':
                if (rowcount != -1)
                    goto USAGE;
                col_name = &argv[0][2];
                if (col_name[0] == '\0') {
                    col_name = argv[1];
                    ++argv, --argc;
                }
                if (my_from_string(col_name, &rowcount))
                    goto USAGE;
                col_name = 0;
                break;
            default:
                goto USAGE;
            }
        }
        else {
            if (col_name)
                goto USAGE;
            col_name = argv[0];
        }
    }
    if (table_path == 0 || col_name == 0) {
    USAGE:
        printf("%s [-s <start row>] [-n <row count>] -i <table> <col>\n", prog);
    }
    else 
        mini_read(table_path, col_name, start, rowcount);

    return 0;
}
