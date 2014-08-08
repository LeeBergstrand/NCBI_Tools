#ifndef ALGO_SEQUENCE___UTIL__HPP
#define ALGO_SEQUENCE___UTIL__HPP

/*  $Id: util.hpp 127313 2008-05-12 12:55:32Z dicuccio $
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
 * Authors:  Christiam Camacho
 *
 * File Description:
 *
 */

#include <corelib/ncbiobj.hpp>
#include <objmgr/scope.hpp>

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects)
class CBioseq;
class CSeq_loc;
END_SCOPE(objects)

// Create virtual Bioseq for masking original sequence with gaps.
// New sequence will has Seq-id 'new_id'
// Its Seq-inst object will be of type delta, and has reference to
// the original sequence ('original_id') and gaps in place of masked ranges.
NCBI_XALGOSEQ_EXPORT
CRef<objects::CBioseq> SeqLocToBioseq(const objects::CSeq_loc& loc,
                                      objects::CScope& scope);

END_NCBI_SCOPE

#endif // ALGO_SEQUENCE___UTIL__HPP
