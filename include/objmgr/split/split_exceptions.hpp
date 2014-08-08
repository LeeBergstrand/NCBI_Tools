#ifndef NCBI_OBJMGR_SPLIT_EXCEPTIONS__HPP
#define NCBI_OBJMGR_SPLIT_EXCEPTIONS__HPP

/*  $Id: split_exceptions.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author:  Eugene Vasilchenko
*
* File Description:
*   Exceptions of split library
*
* ===========================================================================
*/

#include <corelib/ncbiexpt.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


// root class for all object manager exceptions
class NCBI_ID2_SPLIT_EXPORT CSplitException : public CException
{
public:
    enum EErrCode {
        eNotImplemented,    // splitter feature is not implemented yet
        eInvalidBlob,       // invalid source blob
        eCompressionError,  // compression/decompression failed
        eOtherError
    };
    virtual const char* GetErrCodeString(void) const;
    NCBI_EXCEPTION_DEFAULT(CSplitException,CException);
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif//NCBI_OBJMGR_SPLIT_EXCEPTIONS__HPP
