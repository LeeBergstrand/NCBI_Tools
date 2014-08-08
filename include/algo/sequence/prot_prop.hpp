/*  $Id: prot_prop.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * Authors:  Josh Cherry
 *
 * File Description:
 *
 */


#ifndef ALGO_SEQUENCE___PROT_PROP__HPP
#define ALGO_SEQUENCE___PROT_PROP__HPP

#include <objmgr/seq_vector.hpp>
#include <math.h>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

/// This class defines functions for calculating
/// properties of amino acid sequences.
class NCBI_XALGOSEQ_EXPORT CProtProp
{
public:
    /// Counts the numbers of occurrences of each residue type.
    static TSeqPos AACount(CSeqVector& v, vector<TSeqPos>& aacount);

    /// Estimates the isoelectric point (pI) of a protein.
    static double GetProteinPI(CSeqVector& v);

    /// Estimates the charge on a protein
    /// at a given pH.
    static double GetProteinCharge(const vector<TSeqPos>& aacount,
                                   CSeqVector::TResidue nter,
                                   CSeqVector::TResidue cter,
                                   double pH);

private:
    static double exp10(double x) { return pow(double(10), x); }
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif  // ALGO_SEQUENCE___PROT_PROP__HPP
