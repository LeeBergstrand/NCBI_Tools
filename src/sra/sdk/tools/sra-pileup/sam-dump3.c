/*===========================================================================
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
*/
#include "sam-dump.vers.h"

#include <kapp/main.h>
#include <klib/report.h>
#include <sysalloc.h>

#include "sam-dump-opts.h"
#include "inputfiles.h"
#include "sam-hdr.h"
#include "matecache.h"
#include "cg_tools.h"
#include "out_redir.h"
#include "sam-aligned.h"
#include "sam-unaligned.h"


char const *sd_unaligned_usage[]      = { "Output unaligned reads", NULL };

char const *sd_primaryonly_usage[]    = { "Output only primary alignments",
                                       NULL };

char const *sd_cigartype_usage[]      = { "Output long version of CIGAR",
                                       NULL };

char const *sd_cigarCG_usage[]        = { "Output CG version of CIGAR",
                                       NULL };

char const *sd_cigarCGMerge_usage[]   = { "Apply CG fixups to CIGAR/SEQ/QUAL and outputs CG-specific columns",
                                       NULL };

char const *sd_CG_evidence[]          = { "Output CG evidence aligned to reference", 
                                       NULL };

char const *sd_CG_ev_dnb[]            = { "Output CG evidence DNB's aligned to evidence",
                                       NULL };

char const *sd_CG_SAM[]               = { "Output CG evidence DNB's aligned to reference ", 
                                       NULL };

char const *sd_CG_mappings[]          = { "Output CG sequences aligned to reference ",
                                       NULL };

char const *sd_header_usage[]         = { "Always reconstruct header",
                                       NULL };

char const *sd_noheader_usage[]       = { "Do not output headers",
                                       NULL };

char const *sd_region_usage[]         = { "Filter by position on genome.",
                                       "Name can either be file specific name (ex: \"chr1\" or \"1\").",
                                       "\"from\" and \"to\" (inclusive) are 1-based coordinates",
                                       NULL };

char const *sd_distance_usage[]       = { "Filter by distance between matepairs.",
                                       "Use \"unknown\" to find matepairs split between the references.",
                                       "Use from-to (inclusive) to limit matepair distance on the same reference",
                                       NULL };

char const *sd_seq_id_usage[]         = { "Print reference SEQ_ID in RNAME instead of NAME",
                                       NULL };

char const *sd_identicalbases_usage[] = { "Output '=' if base is identical to reference",
                                       NULL };

char const *sd_gzip_usage[]           = { "Compress output using gzip",
                                       NULL };

char const *sd_bzip2_usage[]          = { "Compress output using bzip2",
                                       NULL };

char const *sd_qname_usage[]          = { "Add .SPOT_GROUP to QNAME",
                                       NULL };

char const *sd_fasta_usage[]          = { "Produce Fasta formatted output",
                                       NULL };

char const *sd_fastq_usage[]          = { "Produce FastQ formatted output",
                                       NULL };

char const *sd_prefix_usage[]         = { "Prefix QNAME: prefix.QNAME",
                                       NULL };

char const *sd_reverse_usage[]        = { "Reverse unaligned reads according to read type",
                                       NULL };

char const *sd_comment_usage[]        = { "Add comment to header. Use multiple times for several lines. Use quotes",
                                       NULL };

char const *sd_XI_usage[]             = { "Output cSRA alignment id in XI column",
                                       NULL };

char const *sd_qual_quant_usage[]     = { "Quality scores quantization level",
                                       "a string like '1:10,10:20,20:30,30:-'",
                                       NULL };

char const *sd_report_usage[]         = { "report options instead of executing",
                                       NULL };

char const *sd_outputfile_usage[]     = { "print output into this file (instead of STDOUT)",
                                       NULL };

char const *sd_outbufsize_usage[]     = { "size of output-buffer(dflt:32k, 0...off)",
                                       NULL };

char const *sd_cachereport_usage[]    = { "print report about mate-pair-cache",
                                       NULL };

char const *sd_unaligned_only_usage[] = { "print only unaligned spots",
                                       NULL };

char const *sd_cg_names_usage[]       = { "prints cg-style spotgroup.spotid fromed names",
                                       NULL };

char const *sd_cur_cache_usage[]      = { "open cached cursor with this size",
                                       NULL };

char const *sd_min_mapq_usage[]       = { "min. mapq an alignment has to have, to be printed",
                                       NULL };

OptDef SamDumpArgs[] =
{
    { OPT_UNALIGNED,     "u", NULL, sd_unaligned_usage,      0, false, false },  /* print unaligned reads */
    { OPT_PRIM_ONLY,     "1", NULL, sd_primaryonly_usage,    0, false, false },  /* print only primary alignments */
    { OPT_CIGAR_LONG,    "c", NULL, sd_cigartype_usage,      0, false, false },  /* use long cigar-string instead of short */
    { OPT_CIGAR_CG,     NULL, NULL, sd_cigarCG_usage,        0, false, false },  /* transform cigar into cg-style ( has B/N ) */
    { OPT_RECAL_HDR,     "r", NULL, sd_header_usage,         0, false, false },  /* recalculate header */
    { OPT_NO_HDR,        "n", NULL, sd_noheader_usage,       0, false, false },  /* do not print header */
    { OPT_HDR_COMMENT,  NULL, NULL, sd_comment_usage,        0, true,  false },  /* insert this comment into header */
    { OPT_REGION,       NULL, NULL, sd_region_usage,         0, true,  false },  /* filter by region */
    { OPT_MATE_DIST,    NULL, NULL, sd_distance_usage,       0, true,  false },  /* filter by a list of mate-pair-distances */
    { OPT_USE_SEQID,     "s", NULL, sd_seq_id_usage,         0, false, false },  /* print seq-id instead of seq-name*/
    { OPT_HIDE_IDENT,    "=", NULL, sd_identicalbases_usage, 0, false, false },  /* replace bases that match the reference with '=' */
    { OPT_GZIP,         NULL, NULL, sd_gzip_usage,           0, false, false },  /* compress the output with gzip */
    { OPT_BZIP2,        NULL, NULL, sd_bzip2_usage,          0, false, false },  /* compress the output with bzip2 */
    { OPT_SPOTGRP,       "g", NULL, sd_qname_usage,          0, false, false },  /* add spotgroup to qname */
    { OPT_FASTQ,        NULL, NULL, sd_fastq_usage,          0, false, false },  /* output-format = fastq ( instead of SAM ) */
    { OPT_FASTA,        NULL, NULL, sd_fasta_usage,          0, false, false },  /* output-format = fasta ( instead of SAM ) */
    { OPT_PREFIX,        "p", NULL, sd_prefix_usage,         0, true,  false },  /* prefix QNAME with this string */
    { OPT_REVERSE,      NULL, NULL, sd_reverse_usage,        0, false, false },  /* reverse unaligned reads if reverse-flag set*/
    { OPT_TEST_ROWS,    NULL, NULL, NULL,                    0, true,  false },  /* test-rows, if >0 limit output to this amount of rows */
    { OPT_MATE_GAP,     NULL, NULL, NULL,                    0, true,  false },  /* int value, mate's farther apart than this are not cached */
    { OPT_CIGAR_CG_M,   NULL, NULL, sd_cigarCGMerge_usage,   0, false, false },  /* transform cg-data(length of read/patterns in cigar) into valid SAM (cigar/READ/QUALITY)*/
    { OPT_XI_DEBUG,     NULL, NULL, sd_XI_usage,             0, false, false },  /* output alignment id for debugging... XI:I:NNNNNNN */
    { OPT_Q_QUANT,       "Q", NULL, sd_qual_quant_usage,     0, true,  false },  /* quality-quantization string */
    { OPT_CG_EVIDENCE,  NULL, NULL, sd_CG_evidence,          0, false, false },  /* CG-evidence */
    { OPT_CG_EV_DNB,    NULL, NULL, sd_CG_ev_dnb,            0, false, false },  /* CG-ev-dnb */
    { OPT_CG_MAPP,      NULL, NULL, sd_CG_mappings,          0, false, false },  /* CG-mappings */
    { OPT_CG_SAM,       NULL, NULL, sd_CG_SAM,               0, false, false },  /* cga-tools emulation mode */
    { OPT_REPORT,       NULL, NULL, sd_report_usage,         0, false, false },  /* report options instead of executing */
    { OPT_OUTPUTFILE,   NULL, NULL, sd_outputfile_usage,     0, true,  false },  /* write into output-file */
    { OPT_OUTBUFSIZE,   NULL, NULL, sd_outbufsize_usage,     0, true,  false },  /* size of output-buffer */
    { OPT_CACHEREPORT,  NULL, NULL, sd_cachereport_usage,    0, false, false },  /* report usage of mate-pair-cache */
    { OPT_UNALIGNED_ONLY,NULL, NULL, sd_unaligned_only_usage, 0, false, false },  /* print only unaligned spots */
    { OPT_CG_NAMES,     NULL, NULL, sd_cg_names_usage,       0, false, false },  /* print cg-style spotnames */
    { OPT_CURSOR_CACHE, NULL, NULL, sd_cur_cache_usage,      0, true,  false },  /* size of cursor cache */
    { OPT_MIN_MAPQ,     NULL, NULL, sd_min_mapq_usage,       0, true,  false },  /* minimal mapping quality */
    { OPT_DUMP_MODE,    NULL, NULL, NULL,                    0, true,  false },  /* how to produce aligned reads if no regions given */
    { OPT_CIGAR_TEST,   NULL, NULL, NULL,                    0, true,  false }   /* test cg-treatment of cigar string */
};

char const *sd_usage_params[] =
{
    NULL,                       /* unaligned */
    NULL,                       /* primaryonly */
    NULL,                       /* cigartype */
    NULL,                       /* cigarCG */
    NULL,                       /* recalc header */
    NULL,                       /* no-header */
    "text",                     /* hdr-comment */
    "name[:from-to]",           /* region */
    "from-to|'unknown'",        /* mate distance filter*/
    NULL,                       /* seq-id */
    NULL,                       /* identical-bases */
    NULL,                       /* gzip */
    NULL,                       /* bzip2 */
    NULL,                       /* qname */
    NULL,                       /* fasta */
    NULL,                       /* fastq */
    "prefix",                   /* prefix */
    NULL,                       /* reverse */
    NULL,                       /* test-rows */
    NULL,                       /* mate-row-gap-cacheable */
    NULL,                       /* cigarCGMerge */
    NULL,                       /* XI */
    "quantization string",      /* qual-quant */
    NULL,                       /* CG-evidence */
    NULL,                       /* CG-ev-dnb */
    NULL,                       /* CG-mappings */
    NULL,                       /* CG-SAM */
    NULL,                       /* report */
    NULL,                       /* outputfile */
    NULL,                       /* outputfile buffsize */
    NULL,                       /* cachereport */
    NULL,                       /* unaligned-only */
    NULL,                       /* CG-names */
    NULL,                       /* cursor cache */
    NULL,                       /* min_mapq */
    NULL,                       /* dump_mode */
    NULL                        /* cigar test */
};

ver_t CC KAppVersion( void )
{
    return SAM_DUMP_VERS;
}

const char UsageDefaultName[] = "sam-dump";


rc_t CC UsageSummary( char const *progname )
{
    return KOutMsg( "Usage:\n"
        "\t%s [options] path-to-run[ path-to-run ...]\n\n", progname );
}


rc_t CC Usage( Args const *args )
{
    char const *progname = UsageDefaultName;
    char const *fullpath = UsageDefaultName;
    rc_t rc;
    unsigned i, n;

    rc = ArgsProgram( args, &fullpath, &progname );

    UsageSummary( progname );

    KOutMsg( "Options:\n" );

    n = sizeof( SamDumpArgs ) / sizeof( SamDumpArgs[ 0 ] );
    n--; /* do not print the last option in the SamDumpArgs as help */
    for( i = 0; i < n; i++ )
    {
        if ( SamDumpArgs[ i ].help != NULL )
        {
            HelpOptionLine( SamDumpArgs[ i ].aliases, SamDumpArgs[ i ].name,
                            sd_usage_params[ i ], SamDumpArgs[ i ].help );
        }
    }
    KOutMsg( "\n" );
    HelpOptionsStandard();

    HelpVersion( fullpath, KAppVersion() );

    return rc;
}


/* =========================================================================================== */


static rc_t CC write_to_FILE( void *f, const char *buffer, size_t bytes, size_t *num_writ )
{
    * num_writ = fwrite ( buffer, 1, bytes, f );
    if ( * num_writ != bytes )
        return RC( rcExe, rcFile, rcWriting, rcTransfer, rcIncomplete );
    return 0;
}


/* =========================================================================================== */


static uint32_t tabsel_2_ReferenceList_Options( samdump_opts * opts )
{
    uint32_t res = 0;
    if ( opts->dump_primary_alignments )
        res |= ereferencelist_usePrimaryIds;
    if ( opts->dump_secondary_alignments )
        res |= ereferencelist_useSecondaryIds;
    if ( opts->dump_cg_evidence | opts->dump_cg_ev_dnb )
        res |= ereferencelist_useEvidenceIds;
    return res;
}


static rc_t print_samdump( samdump_opts * opts )
{
    KDirectory *dir;

    rc_t rc = KDirectoryNativeDir( &dir );
    if ( rc != 0 )
    {
        (void)LOGERR( klogErr, rc, "cannot create native directory" );
    }
    else
    {
        const VDBManager *mgr;
        rc = VDBManagerMakeRead( &mgr, dir );
        if ( rc != 0 )
        {
            (void)LOGERR( klogErr, rc, "cannot create vdb-manager" );
        }
        else
        {
            input_files * ifs; /* input_files.h */
            uint32_t reflist_opt = tabsel_2_ReferenceList_Options( opts );
            rc = discover_input_files( &ifs, mgr, opts->input_files, reflist_opt ); /* inputfiles.c */
            if ( rc == 0 )
            {
                if ( ifs->database_count == 0 && ifs->table_count == 0 )
                {
                    rc = RC( rcExe, rcFile, rcReading, rcItem, rcNotFound );
                    (void)LOGERR( klogErr, rc, "input object(s) not found" );
                }
                else
                {
                    matecache *mc;

                    rc = make_matecache( &mc, ifs->database_count );
                    if ( rc == 0 )
                    {
                        /* print output of header */
                        if ( ( opts->output_format == of_sam )  &&
                             ( ifs->database_count > 0 )        &&
                             ( opts->header_mode != hm_none )   &&
                             !opts->dump_unaligned_only ) 
                        /* ------------------------------------------------------ */
                            rc = print_headers( opts, ifs ); /* sam-hdr.c */
                        /* ------------------------------------------------------ */


                        /* print output of aligned reads */
                        if ( rc == 0 && 
                             ifs->database_count > 0 && 
                             !opts->dump_unaligned_only )
                        /* ------------------------------------------------------ */
                            rc = print_aligned_spots( opts, ifs, mc ); /* sam-aligned.c */
                        /* ------------------------------------------------------ */


                        /* print output of unaligned reads */
                        if ( rc == 0 )
                        {
                            /* ------------------------------------------------------ */
                            rc = print_unaligned_spots( opts, ifs, mc ); /* sam-unaligned.c */
                            /* ------------------------------------------------------ */
                        }

                        if ( opts->report_cache )
                            rc = matecache_report( mc ); /* matecache.c */

                        release_matecache( mc ); /* matecache.c */
                    }
                }
                release_input_files( ifs ); /* inputfiles.c */
            }
            VDBManagerRelease( mgr );
        }
        KDirectoryRelease( dir );
    }
    return rc;
}


static rc_t perform_cigar_test( samdump_opts * opts )
{
    rc_t rc;
    cg_cigar_input input;
    cg_cigar_output output;

    memset( &input, 0, sizeof input );
    memset( &output, 0, sizeof output );

    input.cigar_len = strlen( opts->cigar_test );
    input.cigar = opts->cigar_test;

    rc = make_cg_cigar( &input, &output );
    if ( rc == 0 )
    {
        KOutMsg( "%s\n", output.cigar );
    }
    else
    {
        (void)PLOGERR( klogErr, ( klogErr, rc, "error testing cg-cigar treatment '$(t)'", 
                                  "t=%s", opts->cigar_test ) );
    }
    return rc;
}

/* =========================================================================================== */

static rc_t samdump_main( Args * args, samdump_opts * opts )
{
    rc_t rc = 0;
    out_redir redir; /* from out_redir.h */
    enum out_redir_mode mode;

    switch( opts->output_compression )
    {
        case oc_none  : mode = orm_uncompressed; break;
        case oc_gzip  : mode = orm_gzip; break;
        case oc_bzip2 : mode = orm_bzip2; break;
    }

    rc = init_out_redir( &redir, mode, opts->outputfile, opts->output_buffer_size ); /* from out_redir.c */
    if ( rc == 0 )
    {
        if ( opts->report_options )
        {
            report_options( opts ); /* from sam-dump-opts.c */
        }
        else if ( opts->cigar_test != NULL )
        {
            rc = perform_cigar_test( opts );
        }
        else
        {
            if ( opts->input_file_count < 1 )
            {
                rc = RC( rcExe, rcNoTarg, rcConstructing, rcParam, rcInvalid );
                (void)LOGERR( klogErr, rc, "no inputfiles given at commandline" );
                Usage( args );
            }
            else
            {
            /* ------------------------------------------------------ */
                rc = print_samdump( opts );
            /* ------------------------------------------------------ */
            }
        }
        release_out_redir( &redir ); /* from out_redir.c */
    }
    return rc;
}


/* forward declaration, code is in sam-dump.c */
rc_t CC Legacy_KMain( int argc, char* argv[] );


#define LEGACY_FOR_CGI 1

rc_t CC KMain( int argc, char *argv [] )
{
    bool call_legacy_dumper = false;

    rc_t rc = KOutHandlerSet( write_to_FILE, stdout );
    if ( rc != 0 )
    {
        LOGERR( klogInt, rc, "KOutHandlerSet() failed" );
    }
    else
    {
        Args * args;

        KLogHandlerSetStdErr();
        rc = ArgsMakeAndHandle( &args, argc, argv, 1, 
                                SamDumpArgs, sizeof SamDumpArgs / sizeof SamDumpArgs [ 0 ] );
        if ( rc == 0 )
        {
            samdump_opts opts; /* from sam-dump-opts.h */

            memset( &opts, 0, sizeof opts );
            rc = gather_options( args, &opts ); /* from sam-dump-opts.c */
            if ( rc == 0 )
            {
                if ( LEGACY_FOR_CGI > 0 )
                {
                    call_legacy_dumper = (  opts.dump_cg_evidence ||
                                            opts.dump_cg_sam ||
                                            opts.dump_cg_ev_dnb ||
                                            opts.merge_cg_cigar );
                }
                if ( !call_legacy_dumper )
                {
                    ReportBuildDate( __DATE__ );
                    rc = samdump_main( args, &opts );
                }
                /* because the options have sub-structures, like tree/vector's */
                release_options( &opts ); /* from sam-dump-opts.c */
            }
            ArgsWhack( args );
        }
    }

    /* trick to call the legacy sam-dump code if cg-functionality is requested */
    if ( call_legacy_dumper )
    {
        rc = Legacy_KMain( argc, argv );
    }
    else
    {
        /* Report execution environment if necessary */
        rc_t rc2 = ReportFinalize( rc );
        if ( rc == 0 )
            rc = rc2;
    }
    return rc;
}
