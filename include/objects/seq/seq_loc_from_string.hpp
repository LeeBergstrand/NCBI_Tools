#ifndef OBJECTS_SEQ____SEQ_LOC_FROM_STRING__HPP
#define OBJECTS_SEQ____SEQ_LOC_FROM_STRING__HPP

/* $Id: seq_loc_from_string.hpp 346170 2011-12-05 16:04:44Z kornbluh $
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
 * Author:  Mati Shomrat, Michael Kornbluh
 *
 * File Description:
 *   Utilities for converting string to CSeq_loc.
 *
 * ===========================================================================
 */

#include <objects/seqloc/Seq_loc.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

// We don't want a dependency on objmgr, so we have functions
// here that do the best they can without such a dependency.
// The caller is encouraged to override the default functions
// if possible.
class CGetSeqLocFromStringHelper {
public:
    NCBI_SEQ_EXPORT
    virtual ~CGetSeqLocFromStringHelper(void);

    // should act like SeqLocRevCmpl in seq_loc_util.hpp
    // The default implementation does the best it can without a CScope.
    NCBI_SEQ_EXPORT
    virtual CRef<CSeq_loc> GetRevComplement(const CSeq_loc& loc);
    // should act like Seq_loc_Add in seq_loc_util.hpp
    // The default implementation does the best it can without a CScope.
    NCBI_SEQ_EXPORT
    virtual CRef<CSeq_loc> Seq_loc_Add(
        const CSeq_loc&    loc1,
        const CSeq_loc&    loc2,
        CSeq_loc::TOpFlags flags );
};

// for converting strings to locations
NCBI_SEQ_EXPORT
CRef<CSeq_loc> GetSeqLocFromString(
    const string &text, 
    const CSeq_id *id, 
    CGetSeqLocFromStringHelper *helper);

END_SCOPE(objects)
END_NCBI_SCOPE

#endif // OBJECTS_SEQ____SEQ_LOC_FROM_STRING__HPP 

