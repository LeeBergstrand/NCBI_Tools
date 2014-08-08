#ifndef CLASSCTX_HPP
#define CLASSCTX_HPP

/*  $Id: classctx.hpp 122761 2008-03-25 16:45:09Z gouriano $
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
* Author: Eugene Vasilchenko
*
* File Description:
*   Class code generator
*
*/

#include <corelib/ncbistd.hpp>
#include <corelib/ncbistre.hpp>
#include <set>

BEGIN_NCBI_SCOPE

class CDataType;
class CChoiceDataType;
class CFileCode;
class CNamespace;

class CClassContext
{
public:
    virtual ~CClassContext(void);

    typedef set<string> TIncludes;

    virtual string GetMethodPrefix(void) const = 0;
    virtual TIncludes& HPPIncludes(void) = 0;
    virtual TIncludes& CPPIncludes(void) = 0;
    virtual void AddForwardDeclaration(const string& className,
                                       const CNamespace& ns) = 0;
    virtual void AddHPPCode(const CNcbiOstrstream& code) = 0;
    virtual void AddINLCode(const CNcbiOstrstream& code) = 0;
    virtual void AddCPPCode(const CNcbiOstrstream& code) = 0;
    virtual const CNamespace& GetNamespace(void) const = 0;
};

END_NCBI_SCOPE

#endif
