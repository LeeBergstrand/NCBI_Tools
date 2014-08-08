/*  $Id: disc_nucl_options.cpp 116772 2008-01-07 20:49:19Z camacho $
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
 */

/// @file disc_nucl_options.cpp
/// Implements the CDiscNucleotideOptionsHandle class.

#include <ncbi_pch.hpp>
#include <algo/blast/api/disc_nucl_options.hpp>
#include <objects/seqloc/Na_strand.hpp>
#include "blast_setup.hpp"

/** @addtogroup AlgoBlast
 *
 * @{
 */


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(blast)

CDiscNucleotideOptionsHandle::CDiscNucleotideOptionsHandle(EAPILocality locality)
    : CBlastNucleotideOptionsHandle(locality)
{
    SetDefaults();
    m_Opts->SetProgram(eDiscMegablast);
}

void 
CDiscNucleotideOptionsHandle::SetMBLookupTableDefaults()
{
    CBlastNucleotideOptionsHandle::SetMBLookupTableDefaults();
    bool defaults_mode = m_Opts->GetDefaultsMode();
    m_Opts->SetDefaultsMode(false);
    SetTemplateType(0);
    SetTemplateLength(18);
    SetWordSize(BLAST_WORDSIZE_NUCL);
    m_Opts->SetDefaultsMode(defaults_mode);
}

void 
CDiscNucleotideOptionsHandle::SetMBInitialWordOptionsDefaults()
{
    SetXDropoff(BLAST_UNGAPPED_X_DROPOFF_NUCL);
    bool defaults_mode = m_Opts->GetDefaultsMode();
    m_Opts->SetDefaultsMode(false);
    SetWindowSize(BLAST_WINDOW_SIZE_DISC);
    m_Opts->SetDefaultsMode(defaults_mode);
}

void
CDiscNucleotideOptionsHandle::SetMBGappedExtensionDefaults()
{
    SetGapXDropoff(BLAST_GAP_X_DROPOFF_NUCL);
    SetGapXDropoffFinal(BLAST_GAP_X_DROPOFF_FINAL_NUCL);
    SetGapTrigger(BLAST_GAP_TRIGGER_NUCL);
    SetGapExtnAlgorithm(eDynProgScoreOnly);
    SetGapTracebackAlgorithm(eDynProgTbck);
}

void
CDiscNucleotideOptionsHandle::SetMBScoringOptionsDefaults()
{
    CBlastNucleotideOptionsHandle::SetScoringOptionsDefaults();
}

void
CDiscNucleotideOptionsHandle::SetTraditionalBlastnDefaults()
{
    NCBI_THROW(CBlastException, eNotSupported, 
   "Blastn uses a seed extension method incompatible with discontiguous nuclotide blast");
}

END_SCOPE(blast)
END_NCBI_SCOPE


/* @} */
