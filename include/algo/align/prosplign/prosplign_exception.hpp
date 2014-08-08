#ifndef PROSPLIGN_EXCEPTION_HPP
#define PROSPLIGN_EXCEPTION_HPP

/*
*  $Id: prosplign_exception.hpp 107470 2007-07-19 13:35:41Z chetvern $
*
* =========================================================================
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
*  Government do not and cannt warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* =========================================================================
*
*  Author: Boris Kiryutin
*
* =========================================================================
* File Description:
*   prosplign library exceptions
*
*/

#include <corelib/ncbistl.hpp>
#include <corelib/ncbiexpt.hpp>

BEGIN_NCBI_SCOPE

class CProSplignException : EXCEPTION_VIRTUAL_BASE public CException
{
public:
    enum EErrCode {
        eFileNotFound,
        eFormat,
        eOuputError,
        eAliData,
        eBackAli,
        eWrongScore,
        eParam,
        eNotEnoughMemory,
        eGenericError
    };
    virtual const char* GetErrCodeString(void) const {
        switch ( GetErrCode() ) {
        case eFileNotFound:
            return "Can't open file";
        case eFormat:
            return "Unexpected format";
        case eOuputError:
            return "Error in output preparation code";
        case eAliData:
            return "Internal alignment format error";
        case eBackAli:
            return "Back alignment error";
        case eWrongScore:
            return "Score check failed";
        case eParam:
            return "Parameters for alignment are out of scope";
        case eNotEnoughMemory:
            return "Not enough memory error";
        case eGenericError:
            return "Generic error";
        default:
            return CException::GetErrCodeString();
        }
    }
    NCBI_EXCEPTION_DEFAULT(CProSplignException, CException);
};

END_NCBI_SCOPE

#endif  /* PROSPLIGN_EXCEPTION_HPP */
