#ifndef COMMON__TEST_DATA_PATH__H
#define COMMON__TEST_DATA_PATH__H

/* $Id: test_data_path.h 381648 2012-11-27 16:33:11Z gouriano $
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
 * Authors: Andrei Gourianov, Denis Vakatov
 *
 * File Description:
 *   Defines location of test data folder at NCBI
 *
 */

/// @file test_data_path.h
/// Defines location of test data folder at NCBI.

#include <stdlib.h>
#include <string.h>

/// Get the directory where test data is stored at NCBI.
/// The location is hard coded, but can be changed using
/// environment variable NCBI_TEST_DATA_PATH.
///
/// @return
///   Pointer to internal zero-terminated string buffer.
static const char* NCBI_GetTestDataPath(void)
{
    static const char* s_NcbiTestDataPath = NULL;

    if ( s_NcbiTestDataPath )
        return s_NcbiTestDataPath;

    s_NcbiTestDataPath = getenv("NCBI_TEST_DATA_PATH");
    if ( s_NcbiTestDataPath )
        return s_NcbiTestDataPath = strdup(s_NcbiTestDataPath);
    
    s_NcbiTestDataPath =

//#ifdef NCBI_OS_DARWIN
//        "/netopt/toolkit_test_data/"
//#elif defined(NCBI_OS_MSWIN)

#if defined(NCBI_OS_MSWIN)
        "\\\\snowman\\toolkit_test_data\\"
#else
        "/net/snowman/vol/projects/toolkit_test_data/"
#endif
        ;
    return s_NcbiTestDataPath;
}

#endif /* COMMON__TEST_DATA_PATH__H */
