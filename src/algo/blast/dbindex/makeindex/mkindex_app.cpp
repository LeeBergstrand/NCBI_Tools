/*  $Id: mkindex_app.cpp 373169 2012-08-27 14:40:48Z morgulis $
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
 *   Implementation of class CMkIndexApplication.
 *
 */

#include <ncbi_pch.hpp>

#include <memory>
#include <string>
#include <sstream>

#ifdef LOCAL_SVN

#include "../libindexdb_new/sequence_istream_fasta.hpp"
#include "../libindexdb_new/sequence_istream_bdb.hpp"
#include "../libindexdb_new/dbindex.hpp"

#else

#include <algo/blast/dbindex/sequence_istream_fasta.hpp>
#include <algo/blast/dbindex/sequence_istream_bdb.hpp>
#include <algo/blast/dbindex/dbindex.hpp>

#endif

#include "mkindex_app.hpp"

using namespace std;

USING_NCBI_SCOPE;
USING_SCOPE( blastdbindex );

//------------------------------------------------------------------------------
const char * const CMkIndexApplication::USAGE_LINE = 
    "Create a BLAST database index.";

//------------------------------------------------------------------------------
void CMkIndexApplication::Init()
{
    auto_ptr< CArgDescriptions > arg_desc( new CArgDescriptions );
    arg_desc->SetUsageContext( 
            GetArguments().GetProgramBasename(), USAGE_LINE );
    arg_desc->AddOptionalKey( 
            "input", "input_file_name", "input file name",
            CArgDescriptions::eString );
    arg_desc->AddOptionalKey(
            "output", "output_file_name", "output file name",
            CArgDescriptions::eString );
    arg_desc->AddDefaultKey(
            "verbosity", "reporting_level", "how much to report",
            CArgDescriptions::eString, "normal" );
    arg_desc->AddDefaultKey(
            "iformat", "input_format", "type of input used",
            CArgDescriptions::eString, "fasta" );
    arg_desc->AddDefaultKey(
            "legacy", "use_legacy_index_format",
            "use legacy (0-terminated offset lists) dbindex format",
            CArgDescriptions::eBoolean, "true" );
    arg_desc->AddDefaultKey(
            "idmap", "generate_idmap",
            "generate id map for the sequences in the index",
            CArgDescriptions::eBoolean, "false" );
    arg_desc->AddOptionalKey(
            "db_mask", "filtering_algorithm",
            "use the specified filtering algorithm from BLAST DB",
            CArgDescriptions::eInteger );
    arg_desc->AddFlag(
            "show_filters",
            "show the info about available database filtering algorithms"
            " and exit",
            true );
    arg_desc->AddOptionalKey(
            "nmer", "nmer_size",
            "length of the indexed words",
            CArgDescriptions::eInteger );
    arg_desc->AddOptionalKey(
            "ws_hint", "word_size_hint",
            "most likely word size used in searches",
            CArgDescriptions::eInteger );
    arg_desc->AddOptionalKey(
            "volsize", "volume_size", "size of an index volume in MB",
            CArgDescriptions::eInteger );
    arg_desc->AddOptionalKey(
            "stat", "statistics_file",
            "write index statistics into file with that name "
            "(for testing and debugging purposes only).",
            CArgDescriptions::eString );
    arg_desc->AddOptionalKey(
            "stride", "stride",
            "distance between stored database positions",
            CArgDescriptions::eInteger );
    arg_desc->AddDefaultKey(
            "old_style_index", "boolean",
            "Use old style index (deprecated)",
            CArgDescriptions::eBoolean, "true" );
    arg_desc->SetConstraint( 
            "verbosity",
            &(*new CArgAllow_Strings, "quiet", "normal", "verbose") );
    arg_desc->SetConstraint(
            "iformat",
            &(*new CArgAllow_Strings, "fasta", "blastdb") );
    arg_desc->SetConstraint(
            "volsize",
            new CArgAllow_Integers( 1, kMax_Int ) );
    arg_desc->SetConstraint(
            "stride",
            new CArgAllow_Integers( 1, kMax_Int ) );
    arg_desc->SetConstraint(
            "ws_hint",
            new CArgAllow_Integers( 1, kMax_Int ) );
    arg_desc->SetConstraint(
            "nmer",
            new CArgAllow_Integers( 8, 15 ) );
    arg_desc->SetDependency( 
            "show_filters", CArgDescriptions::eExcludes, "output" );
    arg_desc->SetDependency(
            "db_mask", CArgDescriptions::eRequires, "input" );
    SetupArgDescriptions( arg_desc.release() );
}

//------------------------------------------------------------------------------
int CMkIndexApplication::Run()
{ 
    SetDiagPostLevel( eDiag_Warning );
    CDbIndex::SOptions options = CDbIndex::DefaultSOptions();
    std::string verbosity = GetArgs()["verbosity"].AsString();

    bool old_style( GetArgs()["old_style_index"].AsBoolean() );

    if( verbosity == "quiet" ) {
        options.report_level = REPORT_QUIET;
    }else if( verbosity == "verbose" ) {
        options.report_level = REPORT_VERBOSE;
    }

    if( GetArgs()["volsize"] ) {
        options.max_index_size = GetArgs()["volsize"].AsInteger();
    }

    if( GetArgs()["stat"] ) {
        options.stat_file_name = GetArgs()["stat"].AsString();
    }

    if( GetArgs()["nmer"] ) {
        options.hkey_width = GetArgs()["nmer"].AsInteger();
    }

    options.legacy = GetArgs()["legacy"].AsBoolean();
    options.idmap  = GetArgs()["idmap"].AsBoolean();

    if( GetArgs()["stride"] ) {
        if( options.legacy ) {
            ERR_POST( Warning << "-stride has no effect upon "
                                 "legacy index creation" );
        }
        else options.stride = GetArgs()["stride"].AsInteger();
    }

    if( GetArgs()["ws_hint"] ) {
        if( options.legacy ) {
            ERR_POST( Warning << "-ws_hint has no effect upon "
                                 "legacy index creation" );
        }
        else {
            unsigned long ws_hint = GetArgs()["ws_hint"].AsInteger();
    
            if( ws_hint < options.hkey_width + options.stride - 1 ) {
                ws_hint = options.hkey_width + options.stride - 1;
                ERR_POST( Warning << "-ws_hint requested is too low. Setting "
                                     "to the minimum value of " << ws_hint );
            }

            options.ws_hint = ws_hint;
        }
    }

    unsigned int vol_num = 0;

    CDbIndex::TSeqNum start, orig_stop( kMax_UI4 ), stop = 0;
    /*
    string ofname_base = 
        GetArgs()["show_filters"] ? "" : GetArgs()["output"].AsString();
    string odir_name( CFile( ofname_base ).GetDir() );
    */
    CSequenceIStream * seqstream = 0;
    string iformat = GetArgs()["iformat"].AsString();

    if( iformat == "fasta" ) {
        if( GetArgs()["db_mask"] ) {
            ERR_POST( Error << "-db_mask requires -iformat blastdb" );
            exit( 1 );
        }

        if( GetArgs()["input"] ) {
            seqstream = new CSequenceIStreamFasta( 
                    ( GetArgs()["input"].AsString() ) );
        }
        else seqstream = new CSequenceIStreamFasta( NcbiCin );
    }else if( iformat == "blastdb" ) {
        if( GetArgs()["input"] ) {
            if( GetArgs()["show_filters"] ) {
                NcbiCout << CSequenceIStreamBlastDB::ShowSupportedFilters( 
                        GetArgs()["input"].AsString() ) << endl;
                return 0;
            }

            if( old_style ) {
                if( GetArgs()["db_mask"] ) {
                    seqstream = new CSequenceIStreamBlastDB( 
                            GetArgs()["input"].AsString(), true,
                            GetArgs()["db_mask"].AsInteger() );
                }
                else {
                    seqstream = new CSequenceIStreamBlastDB( 
                            GetArgs()["input"].AsString(), false );
                }
            }
        }
        else {
            ERR_POST( Error << "input format 'blastdb' requires -input option" );
            exit( 1 );
        }
    }else {
        ASSERT( 0 );
    }

    if( iformat != "blastdb" && 
            GetArgs()["db_mask"] && 
            GetArgs()["db_mask"].AsString() != "" ) {
        ERR_POST( Error << "option 'db_mask' requires input format 'blastdb'" );
        exit( 1 );
    }

    if( !old_style && iformat == "fasta" ) {
        ERR_POST( Error << "new style index requires input format 'blastdb'" );
        exit( 1 );
    }

    if( !old_style && iformat == "blastdb" ) {
        if( GetArgs()["output"] ) {
            ERR_POST( Warning << 
                      "option 'output' is ignored for new style indices" );
        }

        typedef std::vector< std::string > TStrVec;
        TStrVec db_vols;

        // Enumerate BLAST database volumes.
        {
            std::string ifname( GetArgs()["input"].AsString() );
            CSeqDB db( ifname, CSeqDB::eNucleotide, 0, 0, false );
            db.FindVolumePaths( db_vols, true );
        }

        bool enable_mask( GetArgs()["db_mask"] );
        int filter( enable_mask ? GetArgs()["db_mask"].AsInteger() : 0 );

        ITERATE( TStrVec, dbvi, db_vols ) {
            seqstream = 
                new CSequenceIStreamBlastDB( *dbvi, enable_mask, filter );
            CDbIndex::TSeqNum start, orig_stop( kMax_UI4 ), stop = 0;
            Uint4 vol_num_seq( 0 );

            {
                CSeqDB db( *dbvi, CSeqDB::eNucleotide, 0, 0, false );
                vol_num_seq = db.GetNumOIDs();
            }

            Uint4 num_seq( 0 ), num_vol( 0 );
            vol_num = 0;
            /*
            std::string dbv_name( 
                    CFile::ConcatPath( odir_name, CFile( *dbvi ).GetName() ) );
            */
            std::string dbv_name( *dbvi );
            
            do {
                start = stop;
                stop = orig_stop;
                ostringstream os;
                os << dbv_name << "." << setfill( '0' ) << setw( 2 ) 
                   << vol_num++ << ".idx";
                cerr << "creating " << os.str() << "..." << flush;
                CDbIndex::MakeIndex( 
                        *seqstream, os.str(), start, stop, options );
                num_seq += (stop - start);

                if( start == stop ) cerr << "removed (empty)" << endl;
                else{ 
                    ++num_vol; 
                    cerr << "done" << endl;
                    ERR_POST( Info << 
                              "generated index volume with OIDs: " <<
                              start << "--" << stop );
                }
            }
            while( start != stop );

            if( num_seq != vol_num_seq ) {
                ERR_POST( Error << 
                          "number of sequence reported by BLAST database"
                          " volume (" << vol_num_seq << ") is not the same"
                          " as in the index (" << num_seq << ")" );
                return 1;
            }

            CIndexSuperHeader< 
                CIndexSuperHeader_Base::INDEX_FORMAT_VERSION_1 > shdr( 
                        num_seq, num_vol );
            shdr.Save( dbv_name + ".shd" );
            ERR_POST( Info << 
                      "index generated for BLAST database volume " <<
                      dbv_name << " with " << num_seq << " sequences" );
            delete seqstream;
        }

        return 0;
    }

    Uint4 num_seq( 0 ), num_vol( 0 );
    string ofname_base = 
        GetArgs()["show_filters"] ? "" : GetArgs()["output"].AsString();

    do { 
        start = stop;
        stop = orig_stop;
        ostringstream os;
        os << ofname_base << "." << setfill( '0' ) << setw( 2 ) 
           << vol_num++ << ".idx";
        cerr << "creating " << os.str() << "..." << flush;
        CDbIndex::MakeIndex( 
                *seqstream,
                os.str(), start, stop, options );
        num_seq += (stop - start);

        if( start == stop ) cerr << "removed (empty)" << endl;
        else{ ++num_vol; cerr << "done" << endl; }
    }while( start != stop );

    if( !old_style ) {
        CIndexSuperHeader< 
            CIndexSuperHeader_Base::INDEX_FORMAT_VERSION_1 > shdr(
                    num_seq, num_vol );
        shdr.Save( ofname_base + ".shd" );
    }

    return 0;
}
