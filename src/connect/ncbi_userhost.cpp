/* $Id: ncbi_userhost.cpp 188276 2010-04-08 18:49:38Z grichenk $
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
 * Author:  Aleksey Grichenko
 *
 * File Description:
 *   User and host setup for NCBI Diag
 *
 */

#include <ncbi_pch.hpp>
#include <connect/ncbi_util.h>
#include <connect/ncbi_socket.hpp>
#include <connect/ncbi_userhost.hpp>


BEGIN_NCBI_SCOPE


void SetDiagUserAndHost(TDiagUserAndHost flags)
{
    CDiagContext& ctx = GetDiagContext();
    if ((flags & fDiag_AddUser) != 0  &&
        (((flags & fDiag_OverrideExisting) != 0) ||
        ctx.GetUsername().empty())) {
        const int user_len = 256;
        char user[user_len];
        CORE_GetUsername(user, user_len);
        if ( *user ) {
            GetDiagContext().SetUsername(user);
        }
    }
    if ((flags & fDiag_AddHost) != 0  &&
        (((flags & fDiag_OverrideExisting) != 0)  ||
        ctx.GetHostname().empty())) {
        const string& host = CSocketAPI::gethostname();
        if ( !host.empty() ) {
            GetDiagContext().SetHostname(host);
        }
    }
}


END_NCBI_SCOPE
