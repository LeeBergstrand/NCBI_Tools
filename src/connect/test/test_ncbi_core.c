/* $Id: test_ncbi_core.c 390082 2013-02-23 17:19:30Z lavr $
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
 *   Test suite for:
 *     ncbi_core.[ch]
 *     ncbi_util.[ch]
 *
 */

#include <connect/ncbi_util.h>
#include "../ncbi_ansi_ext.h"
#include "../ncbi_assert.h"
#include <stdlib.h>
#include <errno.h>

#include "test_assert.h"  /* This header must go last */

/* Aux. to printout a name of the next function to test
 */
#define DO_TEST(func)  do {                                             \
        printf("\n----- "#func"  ---------------------------------\n"); \
        func();                                                         \
    } while (0)


/* Some pre-declarations to avoid C++ compiler warnings
 */
#if defined(__cplusplus)
extern "C" {
    static int/*bool*/ TEST_CORE_LockHandler(void* user_data, EMT_Lock how);
    static void        TEST_CORE_LockCleanup(void* user_data);
    static void TEST_CORE_LogHandler(void* user_data, SLOG_Handler* call_data);
    static void TEST_CORE_LogCleanup(void* user_data);
}
#endif /* __cplusplus */



/*****************************************************************************
 *  TEST_CORE -- test "ncbi_core.c"
 */


static void TEST_CORE_Io(void)
{
    /* EIO_Status, IO_StatusStr() */
    int x_status;
    for (x_status = 0;  x_status <= (int) eIO_Unknown;  x_status++) {
        switch ( (EIO_Status) x_status ) {
        case eIO_Success:
        case eIO_Timeout:
        case eIO_Closed:
        case eIO_Interrupt:
        case eIO_InvalidArg:
        case eIO_NotSupported:
        case eIO_Unknown:
            assert(IO_StatusStr((EIO_Status) x_status));
            printf("IO_StatusStr(status = %d): \"%s\"\n",
                   x_status, IO_StatusStr((EIO_Status) x_status));
            break;
        default:
            assert(0);
        }
    }
}


static int TEST_CORE_LockUserData;

/* FMT_LOCK_Handler */
static int/*bool*/ TEST_CORE_LockHandler(void* user_data, EMT_Lock how)
{
    const char* str = 0;
    assert(user_data == &TEST_CORE_LockUserData);

    switch ( how ) {
    case eMT_Lock:
        str = "eMT_Lock";
        break;
    case eMT_LockRead:
        str = "eMT_LockRead";
        break;
    case eMT_Unlock:
        str = "eMT_Unlock";
        break;
    case eMT_TryLock:
        str = "eMT_TryLock";
        break;
    case eMT_TryLockRead:
        str = "eMT_TryLockRead";
        break;
    }
    assert(str);
    printf("TEST_CORE_LockHandler(%s)\n", str);
    return 1/*true*/;
}


/* FMT_LOCK_Cleanup */
static void TEST_CORE_LockCleanup(void* user_data)
{
    assert(user_data == &TEST_CORE_LockUserData);
    printf("TEST_CORE_LockCleanup()\n");
    TEST_CORE_LockUserData = 222;
}


static void TEST_CORE_Lock(void)
{
    /* MT_LOCK API */
    MT_LOCK x_lock;

    /* dummy */
    TEST_CORE_LockUserData = 111;
    x_lock = MT_LOCK_Create(&TEST_CORE_LockUserData,
                            0, TEST_CORE_LockCleanup);
    assert(x_lock);

    verify(MT_LOCK_AddRef(x_lock) == x_lock);
    verify(MT_LOCK_AddRef(x_lock) == x_lock);
    verify(MT_LOCK_Delete(x_lock) == x_lock);
    assert(TEST_CORE_LockUserData == 111);

    verify(MT_LOCK_Do(x_lock, eMT_LockRead));
    verify(MT_LOCK_Do(x_lock, eMT_Lock));
    verify(MT_LOCK_Do(x_lock, eMT_Unlock));
    verify(MT_LOCK_Do(x_lock, eMT_Unlock));

    verify(MT_LOCK_Delete(x_lock) == x_lock);
    assert(TEST_CORE_LockUserData == 111);
    verify(MT_LOCK_Delete(x_lock) == 0);
    assert(TEST_CORE_LockUserData == 222);

    /* real */
    x_lock = MT_LOCK_Create(&TEST_CORE_LockUserData,
                            TEST_CORE_LockHandler, TEST_CORE_LockCleanup);
    assert(x_lock);

    /* NB: Write after read is not usually an allowed lock nesting */
    verify(MT_LOCK_Do(x_lock, eMT_LockRead));
    verify(MT_LOCK_Do(x_lock, eMT_Lock));
    verify(MT_LOCK_Do(x_lock, eMT_Unlock));
    verify(MT_LOCK_Do(x_lock, eMT_Unlock));
    /* Read after write is usually okay */
    verify(MT_LOCK_Do(x_lock, eMT_Lock));
    verify(MT_LOCK_Do(x_lock, eMT_LockRead));
    verify(MT_LOCK_Do(x_lock, eMT_Unlock));
    verify(MT_LOCK_Do(x_lock, eMT_Unlock));
    /* Try-locking sequence */
    verify(MT_LOCK_Do(x_lock, eMT_TryLock));
    verify(MT_LOCK_Do(x_lock, eMT_TryLockRead));
    verify(MT_LOCK_Do(x_lock, eMT_Unlock));
    verify(MT_LOCK_Do(x_lock, eMT_Unlock));

    verify(MT_LOCK_Delete(x_lock) == 0);
}


static int TEST_CORE_LogUserData;

/* FLOG_Handler */
static void TEST_CORE_LogHandler(void* user_data, SLOG_Handler* call_data)
{
    printf("TEST_CORE_LogHandler(round %d):\n", TEST_CORE_LogUserData);
    printf("   Message: %s\n", call_data->message ? call_data->message : "?");
    printf("   Level:   %s\n", LOG_LevelStr(call_data->level));
    printf("   Module:  %s\n", call_data->module ? call_data->module : "?");
    printf("   Func:    %s\n", call_data->func ? call_data->func : "?");
    printf("   File:    %s\n", call_data->file ? call_data->file : "?");
    printf("   Line:    %d\n", call_data->line);
}


/* FLOG_Cleanup */
static void TEST_CORE_LogCleanup(void* user_data)
{
    assert(user_data == &TEST_CORE_LogUserData);
    printf("TEST_CORE_LogCleanup(round %d)\n", TEST_CORE_LogUserData);
    TEST_CORE_LogUserData = 444;
}


static void TEST_CORE_Log(void)
{
    /* LOG */
    LOG x_log;

    /* protective MT-lock */
    MT_LOCK x_lock;

    /* ELOG_Level, LOG_LevelStr() */
    int x_level;
    for (x_level = 0;  x_level <= (int) eLOG_Fatal;  x_level++) {
        switch ( (ELOG_Level) x_level ) {
        case eLOG_Trace:
        case eLOG_Note:
        case eLOG_Warning:
        case eLOG_Error:
        case eLOG_Critical:
        case eLOG_Fatal:
            assert(LOG_LevelStr((ELOG_Level) x_level));
            printf("LOG_LevelStr(level = %d): \"%s\"\n",
                   x_level, LOG_LevelStr((ELOG_Level) x_level));
            break;
        default:
            assert(0);
        }
    }

    /* LOG API */

    /* MT-lock */
    x_lock = MT_LOCK_Create(&TEST_CORE_LockUserData,
                            TEST_CORE_LockHandler, TEST_CORE_LockCleanup);

    /* dummy */
    TEST_CORE_LogUserData = 1;
    x_log = LOG_Create(&TEST_CORE_LogUserData,
                       TEST_CORE_LogHandler, TEST_CORE_LogCleanup,
                       x_lock);
    assert(x_log);

    verify(LOG_AddRef(x_log) == x_log);
    verify(LOG_AddRef(x_log) == x_log);
    verify(LOG_Delete(x_log) == x_log);
    assert(TEST_CORE_LogUserData == 1);

    LOG_WRITE(0, 0, 0, eLOG_Trace, 0);
    LOG_Write(0, 0, 0, eLOG_Trace, 0, 0, 0, 0, 0, 0, 0);
    LOG_WRITE(x_log, 0, 0, eLOG_Trace, 0);
    LOG_Write(x_log, 0, 0, eLOG_Trace, 0, 0, 0, 0, 0, 0, 0);

    verify(LOG_Delete(x_log) == x_log);
    assert(TEST_CORE_LogUserData == 1);

    /* reset to "real" logging */
    LOG_Reset(x_log, &TEST_CORE_LogUserData,
              TEST_CORE_LogHandler, TEST_CORE_LogCleanup);
    assert(TEST_CORE_LogUserData == 444);
    TEST_CORE_LogUserData = 2;

    /* do the test logging */
    LOG_WRITE(x_log, 0, 0, eLOG_Trace, 0);
    LOG_Write(x_log, 0, 0, eLOG_Trace, 0, 0, 0, 0, 0, 0, 0);
    LOG_WRITE(x_log, 0, 0, eLOG_Warning, "");
    /* LOG_WRITE(x_log, eLOG_Fatal, "Something fatal"); */
#undef  THIS_MODULE
#define THIS_MODULE  "SomeModuleName"
    LOG_WRITE(x_log, 0, 0, eLOG_Error, "With changed module name");
#undef  THIS_MODULE
#define THIS_MODULE  0

    /* delete */
    verify(LOG_Delete(x_log) == 0);
    assert(TEST_CORE_LogUserData == 444);
}


static void TEST_CORE(void)
{
    /* Do all TEST_CORE_***() tests */
    DO_TEST(TEST_CORE_Io);
    DO_TEST(TEST_CORE_Lock);
    DO_TEST(TEST_CORE_Log);
}



/*****************************************************************************
 *  TEST_UTIL -- test "ncbi_util.c"
 */

static void TEST_UTIL_Log(void)
{
    /* create */
    LOG x_log = LOG_Create(0, 0, 0, 0);
    LOG_ToFILE(x_log, stdout, 0/*false*/);

    CORE_SetLOGFormatFlags(fLOG_Full | fLOG_Function);

    /* simple logging */
    LOG_WRITE(x_log, 0, 0, eLOG_Trace, 0);
    LOG_Write(x_log, 0, 0, eLOG_Trace, 0, 0, 0, 0, 0, 0, 0);
    LOG_WRITE(x_log, 0, 0, eLOG_Warning, "");
    /* LOG_WRITE(x_log, eLOG_Fatal, "Something fatal"); */
#undef  THIS_MODULE
#define THIS_MODULE  "SomeModuleName"
    LOG_WRITE(x_log, 0, 0, eLOG_Error, "With changed module name");
#undef  THIS_MODULE
#define THIS_MODULE  0

    /* data logging */
    {{
            unsigned char data[300];
            size_t i;
            for (i = 0;  i < sizeof(data);  i++) {
                data[i] = (unsigned char) (i % 256);
            }
            LOG_DATA(x_log, 0, 0, eLOG_Note,
                     data, sizeof(data), "Data logging test");
    }}

    /* delete */
    verify(LOG_Delete(x_log) == 0);
}


/* NOTE: closes STDIN */
static void TEST_CORE_GetUsername(void)
{
    static const char* null =
#ifdef NCBI_OS_MSWIN
        "NUL"
#else
        "/dev/null"
#endif /*NCBI_OS_MSWIN*/
        ;
    char buffer[512];
    const char* temp = CORE_GetUsername(buffer, sizeof(buffer));
    char* user = temp  &&  *temp ? strdup(temp) : 0;
    printf("Username = %s%s%s%s\n",
           temp ? (*temp ? "" : "\"") : "<",
           temp ?   temp              : "NULL",
           temp ? (*temp ? "" : "\"") : ">",
           !temp ^ !user ? ", error!" : "");
    /* NB: GCC's __wur (warn unused result) silenced */
    verify(freopen(null, "r", stdin));  /* NCBI_FAKE_WARNING: GCC */
    temp = CORE_GetUsername(buffer, sizeof(buffer));
    printf("Username = %s%s%s\n",
           temp ? (*temp ? "" : "\"") : "<",
           temp ?   temp              : "NULL",
           temp ? (*temp ? "" : "\"") : ">");
    if (temp  &&  !*temp)
        temp = 0;
    assert((!temp  &&  !user)  ||
           ( temp  &&   user  &&  strcasecmp(temp, user) == 0));
    if (user)
        free(user);
}


static void TEST_UTIL_MatchesMask(void)
{
    assert(UTIL_MatchesMaskEx("aaa", "*a",  0) == 1);
    assert(UTIL_MatchesMaskEx("bbb", "*a",  0) == 0);
    assert(UTIL_MatchesMaskEx("bba", "*a",  0) == 1);
    assert(UTIL_MatchesMaskEx("aab", "*a",  0) == 0);
    assert(UTIL_MatchesMaskEx("aaa", "*a*", 0) == 1);
    assert(UTIL_MatchesMaskEx("AAA", "*a",  1) == 1);
    assert(UTIL_MatchesMaskEx("aaa", "*",   0) == 1);
    printf("PASSED\n");
}


static void TEST_CORE_GetVMPageSize(void)
{
    printf("PageSize = %d\n", (int) CORE_GetVMPageSize());
}


static void TEST_UTIL(void)
{
    DO_TEST(TEST_UTIL_Log);
    DO_TEST(TEST_CORE_GetUsername);
    DO_TEST(TEST_UTIL_MatchesMask);
    DO_TEST(TEST_CORE_GetVMPageSize);
}



/*****************************************************************************
 *  MAIN -- test all
 */

int main(void)
{
    TEST_CORE();
    TEST_UTIL();
    return 0;
}
