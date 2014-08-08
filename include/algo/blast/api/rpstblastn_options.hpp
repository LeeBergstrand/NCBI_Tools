#ifndef ALGO_BLAST_API___RPSTBLASTN_OPTIONS__HPP
#define ALGO_BLAST_API___RPSTBLASTN_OPTIONS__HPP

/*  $Id: rpstblastn_options.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * Authors:  Jason Papadopoulos
 *
 */

/// @file rpstblastn_options.hpp
/// Declares the CRPSTBlastnOptionsHandle class.

#include <algo/blast/api/blast_rps_options.hpp>

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(blast)

/// Handle to the options for translated nucleotide-RPS blast
///
/// Adapter class for translated nucleotide - RPS BLAST searches.
/// Exposes an interface to allow manipulation the options that are relevant to
/// this type of search.

class NCBI_XBLAST_EXPORT CRPSTBlastnOptionsHandle : 
                                            public CBlastRPSOptionsHandle
{
public:

    /// Creates object with default options set
    CRPSTBlastnOptionsHandle(EAPILocality locality = CBlastOptions::eLocal);
    ~CRPSTBlastnOptionsHandle() {}

    /******************* Subject sequence options *******************/
    /// Returns DbGeneticCode
    int GetDbGeneticCode() const {
        return m_Opts->GetDbGeneticCode();
    }
    /// Sets DbGeneticCode
    /// @param gc DbGeneticCode [in]
    void SetDbGeneticCode(int gc) {
        m_Opts->SetDbGeneticCode(gc);
    }

protected:
    /// Set the program and service name for remote blast.
    virtual void SetRemoteProgramAndService_Blast3()
    {
        m_Opts->SetRemoteProgramAndService_Blast3("tblastn", "rpsblast");
    }
    
    /// Overrides SubjectSequenceOptionsDefaults for RPS-TBLASTN options
    void SetQueryOptionDefaults();

private:
    /// Disallow copy constructor
    CRPSTBlastnOptionsHandle(const CRPSTBlastnOptionsHandle& rhs);
    /// Disallow assignment operator
    CRPSTBlastnOptionsHandle& operator=(const CRPSTBlastnOptionsHandle& rhs);
};

END_SCOPE(blast)
END_NCBI_SCOPE


/* @} */

#endif  /* ALGO_BLAST_API___RPSTBLASTN_OPTIONS__HPP */
