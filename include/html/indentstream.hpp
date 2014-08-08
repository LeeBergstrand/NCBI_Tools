#ifndef HTML___INDENTSTREAM__HPP
#define HTML___INDENTSTREAM__HPP

/*  $Id: indentstream.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * Author:  Aaron Ucko
 *
 */

/// @file indentstream.hpp
/// Indenting output stream support.


#include <corelib/ncbistd.hpp>


/** @addtogroup HTMLStream
 *
 * @{
 */


BEGIN_NCBI_SCOPE


class NCBI_XHTML_EXPORT CIndentingOstream : public CNcbiOstream
{
public:
    CIndentingOstream(CNcbiOstream& real_stream, SIZE_TYPE indent = 4);
    ~CIndentingOstream(void) { delete rdbuf(); }
};


END_NCBI_SCOPE


/* @} */

#endif  /* HTML___INDENTSTREAM__HPP */
