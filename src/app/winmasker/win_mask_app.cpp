/*  $Id: win_mask_app.cpp 244879 2011-02-10 17:03:31Z mozese2 $
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
 * Author:  Aleksandr Morgulis
 *
 * File Description:
 *   CWinMaskApplication class member and method definitions.
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbidbg.hpp>
#include <objtools/readers/fasta.hpp>
#include <objects/seqset/Seq_entry.hpp>
#include <objects/seq/Bioseq.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seq/Seq_data.hpp>
#include <objects/seq/seqport_util.hpp>
#include <objects/seq/IUPACna.hpp>
#include <objects/seqloc/Seq_id.hpp>

#include <objmgr/bioseq_ci.hpp>
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/seq_entry_handle.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>

#include <objtools/seqmasks_io/mask_cmdline_args.hpp>
#include <objtools/seqmasks_io/mask_reader.hpp>
#include <objtools/seqmasks_io/mask_fasta_reader.hpp>
#include <objtools/seqmasks_io/mask_writer.hpp>
#include <algo/winmask/seq_masker.hpp>
#include <algo/winmask/win_mask_gen_counts.hpp>
#include <algo/winmask/win_mask_util.hpp>
#include <algo/winmask/win_mask_counts_converter.hpp>
#include <algo/winmask/win_mask_config.hpp>
#include "win_mask_app.hpp"
#include "win_mask_sdust_masker.hpp"


BEGIN_NCBI_SCOPE
USING_SCOPE(objects);

//-------------------------------------------------------------------------
const char * const 
CWinMaskApplication::USAGE_LINE = "Window based sequence masker";

//-------------------------------------------------------------------------
void CWinMaskApplication::Init(void)
{
    HideStdArgs(fHideLogfile | fHideConffile | fHideVersion | fHideDryRun);
    auto_ptr< CArgDescriptions > arg_desc( new CArgDescriptions );

    // Set the program description
    arg_desc->SetUsageContext( GetArguments().GetProgramBasename(),
                               USAGE_LINE );

    CWinMaskConfig::AddWinMaskArgs(*arg_desc);

    // Parse the arguments according to descriptions.
    SetupArgDescriptions(arg_desc.release());
}

//-------------------------------------------------------------------------
int CWinMaskApplication::Run (void)
{
    SetDiagPostLevel( eDiag_Warning );
    CWinMaskConfig aConfig( GetArgs() );

    // Branch away immediately if the converter is called.
    //
    // if( GetArgs()["convert"].AsBoolean() ) {
    if( aConfig.AppType() == CWinMaskConfig::eConvertCounts )
    {
        if( aConfig.Output() == "-" ) {
            CWinMaskCountsConverter converter( 
                    aConfig.Input(),
                    NcbiCout,
                    aConfig.SFormat() );
            return converter();
        }
        else {
            CWinMaskCountsConverter converter( 
                    aConfig.Input(),
                    aConfig.Output(),
                    aConfig.SFormat() );
            return converter();
        }
    }

    CRef<CObjectManager> om(CObjectManager::GetInstance());
    if(aConfig.InFmt() == "seqids")
        CGBDataLoader::RegisterInObjectManager(
            *om, 0, CObjectManager::eDefault );

    // Read and validate configuration values.
    if( aConfig.AppType() == CWinMaskConfig::eComputeCounts )
    {
        if( aConfig.Output() == "-" ) {
            CWinMaskCountsGenerator cg( aConfig.Input(),
                                        NcbiCout,
                                        aConfig.InFmt(),
                                        aConfig.SFormat(),
                                        aConfig.Th(),
                                        aConfig.Mem(),
                                        aConfig.UnitSize(),
                                        aConfig.GenomeSize(),
                                        aConfig.MinScore(),
                                        aConfig.MaxScore(),
                                        aConfig.CheckDup(),
                                        aConfig.FaList(),
                                        aConfig.Ids(),
                                        aConfig.ExcludeIds(),
                                        aConfig.UseBA() );
            cg();
        }
        else {
            CWinMaskCountsGenerator cg( aConfig.Input(),
                                        aConfig.Output(),
                                        aConfig.InFmt(),
                                        aConfig.SFormat(),
                                        aConfig.Th(),
                                        aConfig.Mem(),
                                        aConfig.UnitSize(),
                                        aConfig.GenomeSize(),
                                        aConfig.MinScore(),
                                        aConfig.MaxScore(),
                                        aConfig.CheckDup(),
                                        aConfig.FaList(),
                                        aConfig.Ids(),
                                        aConfig.ExcludeIds(),
                                        aConfig.UseBA() );
            cg();
        }

        return 0;
    }

    if(aConfig.InFmt() == "seqids"){
        LOG_POST(Error << "windowmasker with seqids input not implemented yet");
        return 1;
    }

    CMaskReader & theReader = aConfig.Reader();
    CMaskWriter & theWriter = aConfig.Writer();
    CSeqMasker theMasker( aConfig.LStatName(),
                          aConfig.WindowSize(),
                          aConfig.WindowStep(),
                          aConfig.UnitStep(),
                          aConfig.Textend(),
                          aConfig.CutoffScore(),
                          aConfig.MaxScore(),
                          aConfig.MinScore(),
                          aConfig.SetMaxScore(),
                          aConfig.SetMinScore(),
                          aConfig.MergePass(),
                          aConfig.MergeCutoffScore(),
                          aConfig.AbsMergeCutoffDist(),
                          aConfig.MeanMergeCutoffDist(),
                          aConfig.MergeUnitStep(),
                          aConfig.Trigger(),
                          aConfig.TMin_Count(),
                          aConfig.Discontig(),
                          aConfig.Pattern(),
                          aConfig.UseBA() );
    CRef< CSeq_entry > aSeqEntry( 0 );
    Uint4 total = 0, total_masked = 0;
    CSDustMasker * duster( 0 );
    const CWinMaskConfig::CIdSet * ids( aConfig.Ids() );
    const CWinMaskConfig::CIdSet * exclude_ids( aConfig.ExcludeIds() );

    if( aConfig.AppType() == CWinMaskConfig::eGenerateMasksWithDuster )
        duster = new CSDustMasker( aConfig.DustWindow(),
                                   aConfig.DustLevel(),
                                   aConfig.DustLinker() );

    while( (aSeqEntry = theReader.GetNextSequence()).NotEmpty() )
    {
        if( aSeqEntry->Which() == CSeq_entry::e_not_set ) continue;
        CScope scope(*om);
        CSeq_entry_Handle seh = scope.AddTopLevelSeqEntry(*aSeqEntry);
        Uint4 masked = 0;
        CBioseq_CI bs_iter(seh, CSeq_inst::eMol_na);
        for ( ;  bs_iter;  ++bs_iter) {
            CBioseq_Handle bsh = *bs_iter;
            if (bsh.GetBioseqLength() == 0) {
                continue;
            }

            if( CWinMaskUtil::consider( bsh, ids, exclude_ids ) )
            {
                TSeqPos len = bsh.GetBioseqLength();
                total += len;
                _TRACE( "Sequence length " << len );
                CSeqVector data =
                    bsh.GetSeqVector(CBioseq_Handle::eCoding_Iupac);
                auto_ptr< CSeqMasker::TMaskList > mask_info( theMasker( data ) );
                CSeqMasker::TMaskList dummy;

                if( duster != 0 ) // Dust and merge with mask_info
                {
                    auto_ptr< CSeqMasker::TMaskList > dust_info( 
                        (*duster)( data, *mask_info.get() ) );
                    CSeqMasker::MergeMaskInfo( mask_info.get(), dust_info.get() );
                }

                // theWriter.Print( bsh, *mask_info, aConfig.MatchId() );
                theWriter.Print( bsh, *mask_info, GetArgs()["parse_seqids"] );

                for( CSeqMasker::TMaskList::const_iterator i = mask_info->begin();
                     i != mask_info->end(); ++i )
                    masked += i->second - i->first + 1;

                total_masked += masked;
                _TRACE( "Number of positions masked: " << masked );
            }
        }
    }

    _TRACE( "Total number of positions: " << total );
    _TRACE( "Total number of positions masked: " << total_masked );
    return 0;
}

END_NCBI_SCOPE
