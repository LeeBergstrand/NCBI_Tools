/*  $Id: tblastn_args.hpp 146743 2008-12-01 20:23:50Z camacho $
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
 * Author: Christiam Camacho
 *
 */

/** @file tblastn_args.hpp
 * Main argument class for TBLASTN application
 */

#ifndef ALGO_BLAST_BLASTINPUT___TBLASTN_ARGS__HPP
#define ALGO_BLAST_BLASTINPUT___TBLASTN_ARGS__HPP

#include <algo/blast/blastinput/blast_args.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(blast)

/// Handles command line arguments for blastx binary
class NCBI_BLASTINPUT_EXPORT CTblastnAppArgs : public CBlastAppArgs
{
public:
    /// Constructor
    CTblastnAppArgs();

    /// Get the PSSM
    /// @return non-NULL PSSM if it's psi-tblastn
    CRef<objects::CPssmWithParameters> GetInputPssm() const;

    /// Set the PSSM from the saved search strategy 
    void SetInputPssm(CRef<objects::CPssmWithParameters> pssm);

    /// Get the query batch size
    virtual int GetQueryBatchSize() const;

protected:
    virtual CRef<CBlastOptionsHandle>
    x_CreateOptionsHandle(CBlastOptions::EAPILocality locality,
                          const CArgs& args);

    /// PSI-BLAST specific argument class (for psi-tblastn)
    /// @note this program is added to tblastn because all options for tblastn
    /// apply to psitblastn (i.e.: db genetic code, cannot be iterated)
    CRef<CPsiBlastArgs> m_PsiBlastArgs;
};


END_SCOPE(blast)
END_NCBI_SCOPE

#endif  /* ALGO_BLAST_BLASTINPUT___TBLASTN_ARGS__HPP */
