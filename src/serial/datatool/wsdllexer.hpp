#ifndef WSDLLEXER_HPP
#define WSDLLEXER_HPP

/*  $Id: wsdllexer.hpp 165342 2009-07-09 14:45:07Z gouriano $
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
* Author: Andrei Gourianov
*
* File Description:
*   WSDL lexer
*
* ===========================================================================
*/

#include "xsdlexer.hpp"

BEGIN_NCBI_SCOPE

class WSDLLexer : public XSDLexer
{
public:
    WSDLLexer(CNcbiIstream& in, const string& name);
    virtual ~WSDLLexer(void);

    void UseXSDLexer(bool use)
    {
        m_UseXsd = use;
    }

protected:
    virtual TToken LookupKeyword(void);

private:
    bool m_UseXsd;
};

/////////////////////////////////////////////////////////////////////////////
// WSDLEntityLexer

class WSDLEntityLexer : public WSDLLexer
{
public:
    WSDLEntityLexer(CNcbiIstream& in, const string& name, bool autoDelete=true);
    virtual ~WSDLEntityLexer(void);
protected:
    CNcbiIstream* m_Str;
    bool m_AutoDelete;
};

END_NCBI_SCOPE

#endif // WSDLLEXER_HPP
