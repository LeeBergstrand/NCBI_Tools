/* $Id: test_ncbi_file_connector.c 390082 2013-02-23 17:19:30Z lavr $
 * ===========================================================================
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
 * Author:  Denis Vakatov
 *
 * File Description:
 *   Standard test for the FILE-based CONNECTOR
 *
 */

#include <connect/ncbi_connection.h>
#include <connect/ncbi_file_connector.h>
#include "../ncbi_priv.h"               /* CORE logging facilities */
#include <stdlib.h>

#include "test_assert.h"  /* This header must go last */

#define OUT_FILE "test_ncbi_file_connector.out"


static void Usage(const char* progname, const char* message)
{
    fprintf(stderr,
            "\nUsage: %s <input_file>\n"
            "  (copy <input_file> to \"" OUT_FILE "\")\n"
            "\nERROR:  %s!\n", progname, message);
    abort();
}


int main(int argc, const char* argv[])
{
    CONN        conn;
    CONNECTOR   connector;
    EIO_Status  status;
    const char* inp_file;

    /* cmd.-line args */
    if (argc != 2) {
        Usage(argv[0], "Must specify the input file name");
    }
    inp_file = argv[1];

    /* log and data log streams */
    CORE_SetLOGFormatFlags(fLOG_None          | fLOG_Level   |
                           fLOG_OmitNoteLevel | fLOG_DateTime);
    CORE_SetLOGFILE(stderr, 0/*false*/);

    /* run the test */
    fprintf(stderr,
            "Starting the FILE CONNECTOR test...\n"
            "Copy data from file \"%s\" to file \"%s\".\n\n",
            inp_file, OUT_FILE);

    /* create connector, and bind it to the connection */
    connector = FILE_CreateConnector(inp_file, OUT_FILE);
    if ( !connector ) {
        Usage(argv[0], "Failed to create FILE connector");
    }

    verify(CONN_Create(connector, &conn) == eIO_Success);
 
    /* pump the data from one file to another */
    for (;;) {
        char buf[100];
        size_t n_read, n_written;

        /* read */
        status = CONN_Read(conn, buf, sizeof(buf), &n_read, eIO_ReadPlain);
        if (status != eIO_Success) {
            fprintf(stderr, "CONN_Read() failed (status: %s)\n",
                    IO_StatusStr(status));
            break;
        }
        fprintf(stderr, "READ: %ld  bytes\n", (long) n_read);

        /* write */
        status = CONN_Write(conn, buf, n_read, &n_written, eIO_WritePersist);
        if (status != eIO_Success) {
            fprintf(stderr, "CONN_Write() failed (status: %s)\n",
                    IO_StatusStr(status));
            assert(0);
            break;
        }
        assert(n_written == n_read);
    }
    assert(status == eIO_Closed);
    
    /* cleanup, exit */
    verify(CONN_Close(conn) == eIO_Success);
    CORE_LOG(eLOG_Note, "TEST completed successfully");
    CORE_SetLOG(0);
    return 0;
}
