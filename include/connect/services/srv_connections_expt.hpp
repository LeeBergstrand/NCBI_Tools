#ifndef CONNECT_SERVICES__SRV_CONNECTIONS_EXEPT_HPP
#define CONNECT_SERVICES__SRV_CONNECTIONS_EXEPT_HPP

/*  $Id: srv_connections_expt.hpp 174107 2009-10-23 15:34:40Z kazimird $
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
 * Authors:  Maxim Didneko,
 *
 * File Description:
 *
 */

#include <corelib/ncbiexpt.hpp>


BEGIN_NCBI_SCOPE



/// Net Service exception
///
class CNetSrvConnException : public CException
{
public:
    enum EErrCode {
        eReadTimeout,
        eLBNameNotFound,
        eSrvListEmpty,
        eConnectionFailure,
        eWriteFailure,
        eConnClosedByServer,
        eCommunicationError,
        eServerThrottle,
    };

    virtual const char* GetErrCodeString(void) const
    {
        switch (GetErrCode())
        {
        case eReadTimeout:        return "eReadTimeout";
        case eLBNameNotFound:     return "eLBNameNotFound";
        case eSrvListEmpty:       return "eSrvListEmpty";
        case eConnectionFailure:  return "eConnectionFailure";
        case eWriteFailure:       return "eWriteFailure";
        case eConnClosedByServer: return "eConnClosedByServer";
        case eCommunicationError: return "eCommunicationError";
        case eServerThrottle:     return "eServerThrottle";
        default:                  return CException::GetErrCodeString();
        }
    }

    NCBI_EXCEPTION_DEFAULT(CNetSrvConnException, CException);
};


END_NCBI_SCOPE

#endif // CONNECT_SERVICES__SRV_CONNECTIONS_EXEPT_HPP
