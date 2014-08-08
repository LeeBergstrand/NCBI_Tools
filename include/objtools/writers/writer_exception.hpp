#ifndef OBJTOOLS_WRITERS__WRITER_EXCEPTION_HPP
#define OBJTOOLS_WRITERS__WRITER_EXCEPTION_HPP

/*  $Id: writer_exception.hpp 352815 2012-02-09 15:50:40Z ludwigf $
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
 * Authors:  Josh Cherry
 *
 * File Description:  Exception classes for objtools/writers
 *
 */


#include <corelib/ncbiexpt.hpp>

BEGIN_NCBI_SCOPE

class NCBI_XOBJWRITE_EXPORT CObjWriterException : public CException {
public:
    enum EErrCode {
        /// Argument validation failed
        eArgErr,
        eBadInput,
        eInternal,
    };
    
    virtual const char* GetErrCodeString() const
    {
        switch (GetErrCode()) {
        case eArgErr:
            return "eArgErr";
        case eBadInput:
            return "eBadInput";
        case eInternal:
            return "eInternal";
        default:
            return CException::GetErrCodeString();
        }
    }
    
    /// Include standard NCBI exception behavior.
    NCBI_EXCEPTION_DEFAULT(CObjWriterException, CException);
};

END_NCBI_SCOPE

#endif  // OBJTOOLS_WRITERS__WRITER_EXCEPTION_HPP

