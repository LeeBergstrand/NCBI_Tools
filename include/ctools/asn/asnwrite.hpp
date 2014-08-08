#ifndef CTOOLS__ASN___ASNWRITE__HPP
#define CTOOLS__ASN___ASNWRITE__HPP

/* $Id: asnwrite.hpp 156216 2009-04-01 15:48:45Z lavr $
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
 *   !!! PUT YOUR DESCRIPTION HERE !!!
 */

#include <corelib/ncbistd.hpp>
#include <html/html.hpp>
#include <asn.h>

BEGIN_NCBI_SCOPE

class CAsnWriteNode : public CHTMLNode
{
public:
    CAsnWriteNode(void);
    CAsnWriteNode(int mode); // ASNIO_TEXT or ASNIO_XML
    ~CAsnWriteNode(void);

    AsnIoPtr GetOut(void);
    operator AsnIoPtr(void)
        { return GetOut(); }

    virtual CNcbiOstream& PrintChildren(CNcbiOstream& out, TMode mode);

private:
    // ASN.1 communication interface
    //    static Int2 WriteAsn(Pointer data, CharPtr buffer, Uint2 size);

    // cached ASN.1 communication interface pointer
    AsnIoPtr m_Out;
    int m_Mode;
};

END_NCBI_SCOPE

#endif  /* CTOOLS__ASN___ASNWRITE__HPP */
