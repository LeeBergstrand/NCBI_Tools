#ifndef UTIL_EXCEPTION__HPP
#define UTIL_EXCEPTION__HPP

/* $Id: util_exception.hpp 348644 2012-01-03 15:53:45Z vasilche $
* ===========================================================================
*
*                            public DOMAIN NOTICE                          
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
* Author:  Andrei Gourianov
*
* File Description:
*   Util library exceptions
*
*/

#include <corelib/ncbiexpt.hpp>


/** @addtogroup UtilExcep
 *
 * @{
 */


BEGIN_NCBI_SCOPE


class NCBI_XUTIL_EXPORT CUtilException : public CException
{
public:
    enum EErrCode {
        eNoInput,
        eWrongCommand,
        eWrongData
    };
    virtual const char* GetErrCodeString(void) const;
    NCBI_EXCEPTION_DEFAULT(CUtilException,CException);
};


// this exception is thrown when IO error occurred in serialization
class NCBI_XUTIL_EXPORT CIOException : public CUtilException
{
public:
    enum EErrCode {
        eRead,
        eWrite,
        eFlush,
        eCanceled,
        eOverflow
    };
    virtual const char* GetErrCodeString(void) const;
    NCBI_EXCEPTION_DEFAULT(CIOException, CUtilException);
};

class NCBI_XUTIL_EXPORT CEofException : public CIOException
{
public:
    enum EErrCode {
        eEof
    };
    virtual const char* GetErrCodeString(void) const;
    NCBI_EXCEPTION_DEFAULT(CEofException, CIOException);
};


class NCBI_XUTIL_EXPORT CBlockingQueueException : public CUtilException
{
public:
    enum EErrCode {
        eFull,    // attempt to insert into a full queue
        eTimedOut // Put or WaitForRoom timed out
    };
    virtual const char* GetErrCodeString(void) const;
    NCBI_EXCEPTION_DEFAULT(CBlockingQueueException,CUtilException);
};


END_NCBI_SCOPE


/* @} */

#endif  /* UTIL_EXCEPTION__HPP */
