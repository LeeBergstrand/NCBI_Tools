/* $Id: test_ncbi_http_get.c 398310 2013-05-03 21:12:39Z rafanovi $
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
 * Author:  Anton Lavrentiev
 *
 * File Description:
 *   Retrieve a Web document via HTTP
 *
 */

#include <connect/ncbi_gnutls.h>
#include <connect/ncbi_http_connector.h>
#include <connect/ncbi_util.h>
#include "../ncbi_ansi_ext.h"
#include "../ncbi_priv.h"               /* CORE logging facilities */
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#if defined(NCBI_OS_UNIX)  &&  defined(HAVE_USLEEP)
#  include <unistd.h>
#endif /*NCBI_OS_UNIX && HAVE_USLEEP*/

#include "test_assert.h"  /* This header must go last */


int main(int argc, char* argv[])
{
    CONNECTOR     connector;
    SConnNetInfo* net_info;
    char          blk[512];
    EIO_Status    status;
    THTTP_Flags   flags;
    CONN          conn;
    FILE*         fp;
    time_t        t;
    size_t        n;

    CORE_SetLOGFormatFlags(fLOG_None          | fLOG_Level   |
                           fLOG_OmitNoteLevel | fLOG_DateTime);
    CORE_SetLOGFILE(stderr, 0/*false*/);

    if (argc < 2  ||  !*argv[1])
        CORE_LOG(eLOG_Fatal, "URL has to be supplied on the command line");
    if (argc > 3)
        CORE_LOG(eLOG_Fatal, "Command cannot take more than 2 arguments");
    if (argc == 3) {
        fp = strcmp(argv[2], "-") == 0 ? stdin : fopen(argv[2], "rb");
        if (!fp) {
            CORE_LOGF_ERRNO(eLOG_Error, errno, ("Cannot open \"%s\"",
                                                argv[2] ? argv[2] : ""));
        }
    } else
        fp = 0;

    ConnNetInfo_GetValue(0, "RECONNECT", blk, 32, "");
    if (ConnNetInfo_Boolean(blk)) {
        CORE_LOG(eLOG_Note, "Reconnect mode acknowledged");
        flags = fHTTP_AutoReconnect;
    } else
        flags = 0;

    ConnNetInfo_GetValue(0, "USESSL", blk, 32, "");
    if (ConnNetInfo_Boolean(blk)) {
#ifdef HAVE_LIBGNUTLS
        CORE_LOG(eLOG_Note,    "SSL request acknowledged");
        SOCK_SetupSSL(NcbiSetupGnuTls);
#else
        CORE_LOG(eLOG_Warning, "SSL requested but may be not supported");
#endif /*HAVE_LIBGNUTLS*/
    }

    CORE_LOG(eLOG_Note, "Creating network info structure");
    if (!(net_info = ConnNetInfo_Create(0)))
        CORE_LOG(eLOG_Fatal, "Cannot create network info structure");

    CORE_LOGF(eLOG_Note, ("Parsing URL \"%s\"", argv[1]));
    if (!ConnNetInfo_ParseURL(net_info, argv[1]))
        CORE_LOG(eLOG_Fatal, "URL parse failed");

    CORE_LOGF(eLOG_Note, ("Creating HTTP%s connector",
                          &"S"[net_info->scheme != eURL_Https]));
    if (!(connector = HTTP_CreateConnector(net_info, 0, flags)))
        CORE_LOG(eLOG_Fatal, "Cannot create HTTP connector");

    CORE_LOG(eLOG_Note, "Creating connection");
    if (CONN_Create(connector, &conn) != eIO_Success)
        CORE_LOG(eLOG_Fatal, "Cannot create connection");
    CONN_SetTimeout(conn, eIO_Open,      net_info->timeout);
    CONN_SetTimeout(conn, eIO_ReadWrite, net_info->timeout);
    while (fp  &&  !feof(fp)) {
        n = fread(blk, 1, sizeof(blk), fp);
        status = CONN_Write(conn, blk, n, &n, eIO_WritePersist);
        if (status != eIO_Success) {
            CORE_LOGF(eLOG_Fatal, ("Write error: %s", IO_StatusStr(status)));
        }
    }
    if (fp)
        fclose(fp);

    t = time(0);
    do {
        status = CONN_Wait(conn, eIO_Read, net_info->timeout);
        if (status == eIO_Timeout) {
            if  ((net_info->timeout  &&
                  (net_info->timeout->sec | net_info->timeout->usec))
                 ||  (unsigned long)(time(0) - t) > DEF_CONN_TIMEOUT) {
                CORE_LOG(eLOG_Fatal, "Timed out");
            }
#if defined(NCBI_OS_UNIX)  &&  defined(HAVE_USLEEP)
            usleep(500);
#endif /*NCBI_OS_UNIX && HAVE_USLEEP*/
            continue;
        }
        if (status == eIO_Success)
            status  = CONN_ReadLine(conn, blk, sizeof(blk), &n);
        else
            n = 0;
        if (n) {
            connector = 0/*as bool, visited*/;
            fwrite(blk, 1, n, stdout);
            if (status != eIO_Timeout)
                fputc('\n', stdout);
            fflush(stdout);
            if (n == sizeof(blk)  &&  status != eIO_Closed)
                CORE_LOGF(eLOG_Warning, ("Line too long, continuing..."));
        }
        if (status == eIO_Timeout)
            continue;
        if (status != eIO_Success  &&  (status != eIO_Closed  ||  connector))
            CORE_LOGF(eLOG_Fatal, ("Read error: %s", IO_StatusStr(status)));

    } while (status == eIO_Success  ||  status == eIO_Timeout);

    ConnNetInfo_Destroy(net_info);
    CORE_LOG(eLOG_Note, "Closing connection");
    CONN_Close(conn);

    CORE_LOG(eLOG_Note, "Completed successfully");
    CORE_SetLOG(0);
    return 0;
}
