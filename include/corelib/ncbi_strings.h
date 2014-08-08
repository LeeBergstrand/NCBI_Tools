#ifndef CORELIB___NCBI_STRINGS__HPP
#define CORELIB___NCBI_STRINGS__HPP

/*  $Id: ncbi_strings.h 337341 2011-09-11 01:12:03Z lavr $
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
 * Authors:  Denis Vakatov, Aleksey Grichenko
 *
 *
 */

/**
* @file ncbi_strings.h
*
* String constants used in NCBI C/C++ toolkit.
*
*/

#include <common/ncbi_export.h>


/** @addtogroup String
*
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    eNcbiStrings_Stat,
    eNcbiStrings_PHID
} ENcbiStrings;


NCBI_XNCBI_EXPORT
    const char* g_GetNcbiString(ENcbiStrings what);


#ifdef __cplusplus
}
#endif

#endif  /* CORELIB___NCBI_STRINGS__HPP */

/* @} */
