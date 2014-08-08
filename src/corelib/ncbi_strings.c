/* Hey, Emacs, treat this as -*- C -*- ! */
/*  $Id: ncbi_strings.c 142087 2008-10-02 16:11:16Z grichenk $
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
 * File Description:
 *     String constants used in NCBI C/C++ toolkit.
 *
 */

#include <corelib/ncbi_strings.h>

static const char* s_NcbiStrings[] = {
    "ncbi_st",
    "ncbi_phid"
};


const char* g_GetNcbiString(ENcbiStrings what)
{
    return s_NcbiStrings[what];
}
