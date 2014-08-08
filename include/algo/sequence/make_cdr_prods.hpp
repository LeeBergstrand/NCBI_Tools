/*  $Id: make_cdr_prods.hpp 103491 2007-05-04 17:18:18Z kazimird $
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


#ifndef ALGO_SEQUENCE___MAKE_CDR_PRODS__HPP
#define ALGO_SEQUENCE___MAKE_CDR_PRODS__HPP

#include <objects/seq/Seq_annot.hpp>
#include <objects/seqset/Bioseq_set.hpp>
#include <objmgr/bioseq_handle.hpp>

BEGIN_NCBI_SCOPE

class NCBI_XALGOSEQ_EXPORT CMakeCdrProds
{
public:
    /// Given an annot containing a feature table with Cdregions,
    /// translate those without products.  Make sequence entries
    /// for translation products (returned as a Bioseq_set)
    /// and adjust the Cdregion features to point at them.
    static CRef<objects::CBioseq_set>
    MakeCdrProds(CRef<objects::CSeq_annot> annot,
                 objects::CBioseq_Handle handle);
private:
    static CAtomicCounter sm_Counter;
};


END_NCBI_SCOPE


#endif  // ALGO_SEQUENCE___MAKE_CDR_PRODS__HPP
