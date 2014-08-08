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
#include <kapp/main.h>
#include <klib/log.h>
#include <klib/out.h>
#include <klib/rc.h>
#include <kfs/file.h>
#include <kfs/directory.h>

#include <string.h>

#define SRALOADERFILE_TRACING 1
#define SRALOADERFILE_BUFFERSIZE 20

#include "../../../tools/sra-load/run-xml.h"
#include "../../../tools/sra-load/loader-file.c"

#include "test-loader-file.vers.h"

const char UsageDefaultName[] = "test-loader-file";

rc_t CC UsageSummary( const char * name )
{
    return OUTMSG(( "\nUsage:\n\t%s\n\n", name));
}

rc_t Usage( const Args * args )
{
    const char* progname = UsageDefaultName;
    const char* fullname = progname;

    ArgsProgram(args, &fullname, &progname);
    UsageSummary(progname);
    return 0;
}

uint32_t KAppVersion(void)
{
    return TEST_LOADER_FILE_VERS;
}

rc_t TestRead(KDirectory *dir, const char* fname, const DataBlock* block)
{
    rc_t rc = 0, rc2 = 0;
    const SRALoaderFile* file;
    KFile* kfile = NULL;

    const char* file_data[] = {
        "10\rBs",
        "BBBB10BBBB",
        "", /* no advance */
        "CCCCC\nC17CCnCCCCC",
        "\nthis and next lines are too long for buffer\n",
        "DDDDD\rDD23D\rDDDDD\rDDDDD",
        "Last Block!!",
        NULL
        };

    /* writing test data */
    LOGMSG(klogInfo, "Testing SRALoaderFileRead");
    if( (rc = KDirectoryCreateFile(dir, &kfile, false, 0666, kcmOpen | kcmInit, fname)) != 0 ) {
        LOGERR(klogErr, rc, "KDirectoryOpenFileWrite");
    } else {
        if( (rc = KFileSetSize(kfile, 0)) != 0 ) {
            LOGERR(klogErr, rc, "KFileSetSize");
        } else {
            size_t writ = 0;
            uint64_t i = -1, pos = 0;
            while( rc == 0 && file_data[++i] != NULL ) {
                int len = strlen(file_data[i]);
                if( (rc = KFileWrite(kfile, pos, file_data[i], len, &writ)) != 0 ) {
                    LOGERR(klogErr, rc, "KFileWrite");
                } else if( writ != len ) {
                    rc = RC(rcExe, rcFormatter, rcWriting, rcFile, rcIncomplete);
                    LOGERR(klogErr, rc, "strlen != num_written");
                }
                pos += len;
            }
        }
        KFileRelease(kfile);
    }

    /* reading lines */
    if( rc == 0 ) {
        if( (rc = SRALoaderFile_Make(&file, dir, strdup(fname), block, NULL, NULL, false)) != 0 ) {
            LOGERR(klogErr, rc, "SRALoaderFile_Make");
        } else {
            const void* buf;
            char big_buff[SRALOADERFILE_BUFFERSIZE * 100];
            size_t len, request = 0, advance = 0;
            int i = -1;

            big_buff[0] = '\0';
            while( rc == 0 && file_data[++i] != NULL ) {
                request = strlen(file_data[i]);
                do {
                    if( (rc = SRALoaderFileRead(file, advance, request, &buf, &len)) == 0 ) {
                        if( buf == NULL ) {
                            LOGERR(klogInfo, rc, "@EOF");
                            break;
                        } else if( request > 0 && request != len ) {
                            rc = RC(rcExe, rcFormatter, rcValidating, rcFile, rcTooShort);
                            PLOGERR(klogErr, (klogErr, rc, "read requested $(requested) - got $(got) bytes",
                                    PLOG_2(PLOG_U32(request),PLOG_U32(got)), request, len));
                        } else {
                            strncat(big_buff, buf, request);
                            if( strcmp(big_buff, file_data[i]) == 0 ) {
                                PLOGMSG(klogInfo, (klogInfo, "read '$(line)' - orig '$(orig)'",
                                        PLOG_2(PLOG_S(line),PLOG_S(orig)), big_buff, file_data[i]));
                            } else {
                                rc = RC(rcExe, rcFormatter, rcValidating, rcFile, rcCorrupt);
                                PLOGERR(klogErr, (klogErr, rc, "read '$(line)' - orig '$(orig)'",
                                        PLOG_2(PLOG_S(line),PLOG_S(orig)), big_buff, file_data[i]));
                            }
                            len = request > 0 ? len : 0; /* reset len if nothing was requested */
                            memset(big_buff, 0, sizeof(big_buff));
                        }
                    } else if( GetRCObject(rc) == rcBuffer && GetRCState(rc) == rcInsufficient ) {
                        PLOGMSG(klogWarn, (klogWarn, "Long chunk - double buffering $(length)", "length=%u", len));
                        rc = 0;
                        strncat(big_buff, buf, len);
                    } else {
                        LOGERR(klogErr, rc, "SRALoaderFileRead");
                    }
                    request -= len;
                    advance = len;
                } while( rc == 0 && request > 0 );
            }
            if( rc == 0 ) {
                PLOGMSG(klogInfo, (klogInfo, "Read $(n) blocks", PLOG_U32(n), i));
                if( i != (sizeof(file_data) / sizeof(file_data[0])) - 1 ) {
                    rc = RC(rcExe, rcFormatter, rcValidating, rcFile, rcIncomplete);
                    PLOGERR(klogErr, (klogErr, rc, "$(n) blocks left unread", PLOG_U32(n), i - (sizeof(file_data) / sizeof(file_data[0])) - 1));
                } else if( (rc = SRALoaderFileRead(file, advance, 0, &buf, &len)) != 0 ) {
                    LOGERR(klogErr, rc, "SRALoaderFileRead");
                } else if( buf != NULL ) {
                    rc = RC(rcExe, rcFormatter, rcValidating, rcFile, rcExcessive);
                    LOGERR(klogErr, rc, "expected EOF");
                } else {
                    LOGMSG(klogInfo, "EOF as expected");
                }
            }
        }
    }
    if( rc == 0 ) {
        bool eof = false;
        if( (rc = SRALoaderFile_IsEof(file, &eof)) == 0 ) {
            if( eof != true ) {
                const char* fn = NULL;
                if( (rc = SRALoaderFileName(file, &fn)) == 0 ) {
                    rc = RC(rcExe, rcFormatter, rcReading, rcFile, rcExcessive);
                    PLOGERR(klogErr, (klogErr, rc, "File $(file) not completely parsed", PLOG_S(file), fn));
                } else {
                    LOGERR(klogErr, rc, "SRALoaderFileName");
                }
            }
        } else {
            LOGERR(klogErr, rc, "SRALoaderFile_IsEof");
        }
    }
    if( (rc2 = SRALoaderFile_Release(file)) != 0 ) {
        LOGERR(klogErr, rc2, "SRALoaderFile_Release");
        rc = rc ? rc : rc2;
    }
    return rc;
}

rc_t TestReadLine(KDirectory *dir, const char* fname, const DataBlock* block)
{
    rc_t rc = 0, rc2 = 0;
    const SRALoaderFile* file;
    KFile* kfile = NULL;

    const char* file_data[][2] = {
        {"short line", "\n"},
        {"next line is empty", "\n"},
        {"", "\n"},
        {"buf-1 sz (19 chars)", "\n"},
        {"buf sz!!! (20 chars)", "\n"},
        {"mac os separated", "\r"},
        {"win separated", "\r\n"},
        {"\\r&\\n split by buffer", "\r\n"},
        {"next NOT EMPTY line after \\r\\n", "\r"},
        {"this line is EOF and no \\n", ""},
        {NULL, NULL}
        };

    /* writing test data */
    LOGMSG(klogInfo, "Testing SRALoaderFileReadline");
    if( (rc = KDirectoryCreateFile(dir, &kfile, false, 0666, kcmOpen | kcmInit, fname)) != 0 ) {
        LOGERR(klogErr, rc, "KDirectoryOpenFileWrite");
    } else {
        if( (rc = KFileSetSize(kfile, 0)) != 0 ) {
            LOGERR(klogErr, rc, "KFileSetSize");
        } else {
            size_t writ = 0;
            uint64_t i = -1, j, pos = 0;
            while( rc == 0 && file_data[++i][0] != NULL ) {
                for(j = 0; rc == 0 && j < 2; j++) {
                    int len = strlen(file_data[i][j]);
                    if( (rc = KFileWrite(kfile, pos, file_data[i][j], len, &writ)) != 0 ) {
                        LOGERR(klogErr, rc, "KFileWrite");
                    } else if( writ != len ) {
                        rc = RC(rcExe, rcFormatter, rcWriting, rcFile, rcIncomplete);
                        LOGERR(klogErr, rc, "strlen != num_written");
                    }
                    pos += len;
                }
            }
        }
        KFileRelease(kfile);
    }

    /* reading lines */
    if( rc == 0 ) {
        if( (rc = SRALoaderFile_Make(&file, dir, strdup(fname), block, NULL, NULL, false)) != 0 ) {
            LOGERR(klogErr, rc, "SRALoaderFile_Make");
        } else {
            const void* buf;
            char long_line[SRALOADERFILE_BUFFERSIZE * 100];
            size_t len;
            int i = -1;

            long_line[0] = '\0';
            while( rc == 0 && file_data[++i][0] != NULL ) {
                if( (rc = SRALoaderFileReadline(file, &buf, &len)) == 0 ) {
                    if( buf == NULL ) {
                        LOGERR(klogInfo, rc, "@EOF");
                        break;
                    } else {
                        strncat(long_line, buf, len);
                        if( strcmp(long_line, file_data[i][0]) == 0 ) {
                            PLOGMSG(klogInfo, (klogInfo, "read '$(line)' - orig '$(orig)'",
                                    PLOG_2(PLOG_S(line),PLOG_S(orig)), long_line, file_data[i][0]));
                        } else {
                            rc = RC(rcExe, rcFormatter, rcValidating, rcFile, rcCorrupt);
                            PLOGERR(klogErr, (klogErr, rc, "read '$(line)' - orig '$(orig)'",
                                    PLOG_2(PLOG_S(line),PLOG_S(orig)), long_line, file_data[i][0]));

                        }
                    }
                    memset(long_line, 0, sizeof(long_line));
                } else if( GetRCState(rc) == rcTooLong ) {
                    PLOGMSG(klogWarn, (klogWarn, "Long line - double buffering $(length)", "length=%u", len));
                    memcpy(long_line, buf, len);
                    rc = 0;
                    i--;
                } else {
                    LOGERR(klogErr, rc, "SRALoaderFileReadline");
                }
            }
            if( rc == 0 ) {
                PLOGMSG(klogInfo, (klogInfo, "Read $(n) lines", PLOG_U32(n), i));
                if( i != (sizeof(file_data) / sizeof(file_data[0])) - 1 ) {
                    rc = RC(rcExe, rcFormatter, rcValidating, rcFile, rcIncomplete);
                    PLOGERR(klogErr, (klogErr, rc, "$(n) lines left unread", PLOG_U32(n), i - (sizeof(file_data) / sizeof(file_data[0])) - 1));
                } else if( (rc = SRALoaderFileReadline(file, &buf, &len)) != 0 ) {
                    LOGERR(klogErr, rc, "SRALoaderFileReadline");
                } else if( buf != NULL ) {
                    rc = RC(rcExe, rcFormatter, rcValidating, rcFile, rcExcessive);
                    LOGERR(klogErr, rc, "expected EOF");
                } else {
                    LOGMSG(klogInfo, "EOF as expected");
                }
            }
        }
    }
    if( rc == 0 ) {
        bool eof = false;
        if( (rc = SRALoaderFile_IsEof(file, &eof)) == 0 ) {
            if( eof != true ) {
                const char* fn = NULL;
                if( (rc = SRALoaderFileName(file, &fn)) == 0 ) {
                    rc = RC(rcExe, rcFormatter, rcReading, rcFile, rcExcessive);
                    PLOGERR(klogErr, (klogErr, rc, "File $(file) not completely parsed", PLOG_S(file), fn));
                } else {
                    LOGERR(klogErr, rc, "SRALoaderFileName");
                }
            }
        } else {
            LOGERR(klogErr, rc, "SRALoaderFile_IsEof");
        }
    }
    if( (rc2 = SRALoaderFile_Release(file)) != 0 ) {
        LOGERR(klogErr, rc2, "SRALoaderFile_Release");
        rc = rc ? rc : rc2;
    }
    return rc;
}

rc_t KMain(int argc, char *argv[])
{
    rc_t rc = 0, rc2 = 0;
    KDirectory *dir = NULL;
    DataBlock block;
    const char* ftest_name[2] = {"test-loader-file.txt", "test-loader-file.dat"};


    KDbgSetString("APP");
    KLogLevelSet(klogLevelMax);
    LOGMSG(klogInfo, "START");
    memset(&block, 0, sizeof(block));

    if( (rc = KDirectoryNativeDir(&dir)) != 0 ) {
        LOGERR(klogErr, rc, "KDirectoryNativeDir");
    } else {
        if( (rc = TestReadLine(dir, ftest_name[0], &block)) != 0 ) {
            PLOGERR(rc ? klogErr : klogInfo, (rc ? klogErr : klogInfo, rc,
                     "READLINE TEST $(status)", PLOG_S(status), rc != 0 ? "FAILED" : "PASSED"));
        }
        if( (rc = TestRead(dir, ftest_name[1], &block)) != 0 ) {
            PLOGERR(rc ? klogErr : klogInfo, (rc ? klogErr : klogInfo, rc,
                     "READ TEST $(status)", PLOG_S(status), rc != 0 ? "FAILED" : "PASSED"));
        }
    }
    if( rc == 0 ) {
        rc = KDirectoryRemove(dir, true, ftest_name[0]);
        rc2 = KDirectoryRemove(dir, true, ftest_name[1]);
        rc = rc ? rc : rc2;
        if( rc != 0 ) {
            LOGERR(klogErr, rc, "KDirectoryRemove");
        }
    }
    if( (rc2 = KDirectoryRelease(dir)) != 0 ) {
        LOGERR(klogErr, rc2, "KDirectoryRelease");
        rc = rc ? rc : rc2;
    }
    PLOGERR(rc ? klogErr : klogInfo, (rc ? klogErr : klogInfo, rc,
             "ALL TESTS $(status)", PLOG_S(status), rc != 0 ? "FAILED" : "PASSED"));
    return rc;
}
