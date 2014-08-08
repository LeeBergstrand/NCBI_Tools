#ifndef UTIL___ASCII85__HPP
#define UTIL___ASCII85__HPP

/*  $Id: ascii85.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * Author: Peter Meric
 *
 * File Description:
 *    ASCII base-85 conversion functions
 *
 */

#include <corelib/ncbistd.hpp>


BEGIN_NCBI_SCOPE


class NCBI_XUTIL_EXPORT CAscii85
{
public:
    static size_t s_Encode(const char* src_buf, size_t src_len,
                           char* dst_buf, size_t dst_len
                          );
};


END_NCBI_SCOPE

#endif  // UTIL___ASCII85__HPP
