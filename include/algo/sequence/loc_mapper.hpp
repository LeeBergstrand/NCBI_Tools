/*  $Id: loc_mapper.hpp 130252 2008-06-09 16:35:10Z astashya $
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
 * Authors:  Alex Astashyn
 *
 * File Description:
 *
 */

#ifndef LOC_MAPPER_HPP_
#define LOC_MAPPER_HPP_

#include <objects/seqloc/Seq_loc.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);

///////////////////////////////////////////////////////////////////////////////

class ILocMapper : public CObject
{
public:
    virtual CRef<CSeq_loc> Map(const CSeq_loc& loc, double* remapped_identity_out = NULL) = 0;
    virtual ~ILocMapper() {}
};

END_NCBI_SCOPE

#endif
