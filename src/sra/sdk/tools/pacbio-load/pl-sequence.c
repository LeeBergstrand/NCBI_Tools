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

#include "pl-sequence.h"
#include "pl-regions.h"
#include <kfs/kfs-priv.h>       /* access to getmeta of KArrayfile*/
#include <sra/pacbio.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* enumeration of the columns of the pulse-table */
enum
{
    /* base-space */
    seq_tab_READ = 0,
    seq_tab_QUALITY,
    seq_tab_NREADS,
    seq_tab_PLATFORM,

    seq_tab_LABEL,
    seq_tab_LABEL_START,
    seq_tab_LABEL_LEN,
    seq_tab_READ_TYPE,
    seq_tab_READ_START,
    seq_tab_READ_LEN,
    seq_tab_CLIP_QUALITY_LEFT,
    seq_tab_CLIP_QUALITY_RIGHT,
    seq_tab_READ_FILTER,

    /* pulse-space */
    seq_tab_PRE_BASE_FRAMES,
    seq_tab_WIDTH_IN_FRAMES,
    seq_tab_PULSE_INDEX_16,
    seq_tab_PULSE_INDEX_32,
    seq_tab_HOLE_NUMBER,
    seq_tab_HOLE_STATUS,
    seq_tab_HOLE_XY,
    seq_tab_INSERTION_QV,
    seq_tab_DELETION_QV,
    seq_tab_DELETION_TAG,
    seq_tab_SUBSTITUTION_QV,
    seq_tab_SUBSTITUTION_TAG,
    seq_tab_count
};

const char * seq_tab_names[] = 
{ 
    /* base-space */
    "READ",
    "QUALITY",
    "NREADS",
    "(INSDC:SRA:platform_id)PLATFORM",
    "LABEL",
    "LABEL_START",
    "LABEL_LEN",
    "READ_TYPE",
    "READ_START",
    "READ_LEN",
    "CLIP_QUALITY_LEFT",
    "CLIP_QUALITY_RIGHT",
    "READ_FILTER",

    /* pulse-space */
    "PRE_BASE_FRAMES",
    "WIDTH_IN_FRAMES",
    "(U16)PULSE_INDEX",
    "PULSE_INDEX",
    "HOLE_NUMBER",
    "HOLE_STATUS",
    "HOLE_XY",
    "INSERTION_QV",
    "DELETION_QV",
    "DELETION_TAG",
    "SUBSTITUTION_QV",
    "SUBSTITUTION_TAG"
};

typedef struct BaseCalls
{
    BaseCalls_cmn cmn;
    regions rgn;
    af_data PreBaseFrames;
    af_data PulseIndex;
    af_data WidthInFrames;
} BaseCalls;


static void init_BaseCalls( BaseCalls *tab )
{
    init_BaseCalls_cmn( &tab->cmn );
    rgn_init( &tab->rgn );
    init_array_file( &tab->PreBaseFrames );
    init_array_file( &tab->PulseIndex );
    init_array_file( &tab->WidthInFrames );
}


static void close_BaseCalls( BaseCalls *tab )
{
    close_BaseCalls_cmn( &tab->cmn );
    rgn_free( &tab->rgn );
    free_array_file( &tab->PreBaseFrames );
    free_array_file( &tab->PulseIndex );
    free_array_file( &tab->WidthInFrames );
}


static rc_t open_BaseCalls( const KDirectory *hdf5_dir, BaseCalls *tab,
                            const char * path, bool cache_content, bool * rgn_present )
{
    rc_t rc, rc1;

    init_BaseCalls( tab );
    rc = open_BaseCalls_cmn( hdf5_dir, &tab->cmn, false, path, cache_content, false );
    if ( rc == 0 )
    {
        rc1 = rgn_open( hdf5_dir, &tab->rgn );
        if ( rgn_present != NULL )
        {
            *rgn_present = ( rc1 == 0 );
        }
    }
    if ( rc == 0 )
        rc = open_element( hdf5_dir, &tab->PreBaseFrames, path, "PreBaseFrames", 
                           PRE_BASE_FRAMES_BITSIZE, PRE_BASE_FRAMES_COLS,
                           true, cache_content, false );
    if ( rc == 0 )
    {
        rc = open_element( hdf5_dir, &tab->PulseIndex, path, "PulseIndex", 
                           PULSE_INDEX_BITSIZE_16, PULSE_INDEX_COLS,
                           false, cache_content, false );
        /* try again if it is not 16 bit! */
        if ( rc != 0 )
            rc = open_element( hdf5_dir, &tab->PulseIndex, path, "PulseIndex", 
                               PULSE_INDEX_BITSIZE_32, PULSE_INDEX_COLS,
                               true, cache_content, false );
    }

    if ( rc == 0 )
        rc = open_element( hdf5_dir, &tab->WidthInFrames, path, "WidthInFrames",
                           WIDTH_IN_FRAMES_BITSIZE, WIDTH_IN_FRAMES_COLS,
                           true, cache_content, false );
    if ( rc != 0 )
        close_BaseCalls( tab ); /* releases only initialized elements */
    return rc;
}


static bool check_BaseCall_totalcount( BaseCalls *tab, const uint64_t expected )
{
    bool res = check_table_count( &tab->PreBaseFrames, "PreBaseFrames", expected );
    if ( res )
        res = check_table_count( &tab->PulseIndex, "PulseIndex", expected );
    if ( res )
        res = check_table_count( &tab->WidthInFrames, "WidthInFrames", expected );
    if ( res )
        res = check_table_count( &tab->cmn.Basecall, "Basecall", expected );
    if ( res )
        res = check_table_count( &tab->cmn.DeletionQV, "DeletionQV", expected );
    if ( res )
        res = check_table_count( &tab->cmn.DeletionTag, "DeletionTag", expected );
    if ( res )
        res = check_table_count( &tab->cmn.InsertionQV, "InsertionQV", expected );
    if ( res )
        res = check_table_count( &tab->cmn.QualityValue, "QualityValue", expected );
    if ( res )
        res = check_table_count( &tab->cmn.SubstitutionQV, "SubstitutionQV", expected );
    if ( res )
        res = check_table_count( &tab->cmn.SubstitutionTag, "SubstitutionTag", expected );
    return res;
}


static rc_t seq_load_zero_bases( VCursor *cursor, BaseCalls *tab,
                                 const uint32_t *col_idx )
{
    uint32_t dummy_src;
    rc_t rc = vdb_write_value( cursor, col_idx[ seq_tab_READ ],
                               &dummy_src, BASECALL_BITSIZE, 0, "seq.Basecall" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_QUALITY ],
                              &dummy_src, QUALITY_VALUE_BITSIZE, 0, "seq.QualityValue" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_INSERTION_QV ],
                              &dummy_src, INSERTION_QV_BITSIZE, 0, "seq.InsertionQV" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_DELETION_QV ],
                              &dummy_src, DELETION_QV_BITSIZE, 0, "seq.DeletionQV" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_DELETION_TAG ],
                              &dummy_src, DELETION_TAG_BITSIZE, 0, "seq.DeletionTag" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_SUBSTITUTION_QV ],
                              &dummy_src, SUBSTITUTION_QV_BITZISE, 0, "seq.SubstitutionQV" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_SUBSTITUTION_TAG ],
                              &dummy_src, SUBSTITUTION_TAG_BITSIZE, 0, "seq.SubstitutionTag" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_PRE_BASE_FRAMES ],
                              &dummy_src, PRE_BASE_FRAMES_BITSIZE, 0, "seq.PreBaseFrames" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_WIDTH_IN_FRAMES ],
                              &dummy_src, WIDTH_IN_FRAMES_BITSIZE, 0, "seq.WidthInFrames" );
    if ( rc == 0 )
    {
        if ( tab->PulseIndex.element_bits == PULSE_INDEX_BITSIZE_16 )
            rc = vdb_write_value( cursor, col_idx[ seq_tab_PULSE_INDEX_16 ],
                                  &dummy_src, PULSE_INDEX_BITSIZE_16, 0, "seq.PulseIndex16" );
        else
            rc = vdb_write_value( cursor, col_idx[ seq_tab_PULSE_INDEX_32 ],
                                  &dummy_src, PULSE_INDEX_BITSIZE_32, 0, "seq.PulseIndex32" );
    }
    return rc;
}


static rc_t seq_load_spot_bases( VCursor *cursor, BaseCalls *tab,
                                 const uint32_t *col_idx, zmw_row * spot )
{
    rc_t rc = 0;
    size_t n_bytes = spot->NumEvent * 4;
    /* we make a buffer to store NumEvent 32-bit-values
      (that is so far the biggest value we have to read per DNA-BASE) */
    char * buffer = malloc( n_bytes );
    if ( buffer == NULL )
    {
        rc = RC( rcExe, rcNoTarg, rcAllocating, rcMemory, rcExhausted );
        PLOGERR( klogErr, ( klogErr, rc, "cannot allocate $(nbytes) bytes to read seq-data",
                            "nbytes=%u", n_bytes ) );
    }
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_READ ],
            &tab->cmn.Basecall, buffer, spot->offset, spot->NumEvent,
            BASECALL_BITSIZE, "seq.Basecall" );
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_QUALITY ],
            &tab->cmn.QualityValue, buffer, spot->offset, spot->NumEvent,
            QUALITY_VALUE_BITSIZE, "seq.QualityValue" );
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_INSERTION_QV ],
            &tab->cmn.InsertionQV, buffer, spot->offset, spot->NumEvent,
            INSERTION_QV_BITSIZE, "seq.InsertionQV" );
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_DELETION_QV ],
            &tab->cmn.DeletionQV, buffer, spot->offset, spot->NumEvent,
            DELETION_QV_BITSIZE, "seq.DeletionQV" );
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_DELETION_TAG ],
            &tab->cmn.DeletionTag, buffer, spot->offset, spot->NumEvent,
            DELETION_TAG_BITSIZE, "seq.DeletionTag" );
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_SUBSTITUTION_QV ],
            &tab->cmn.SubstitutionQV, buffer, spot->offset, spot->NumEvent,
            SUBSTITUTION_QV_BITZISE, "seq.SubstitutionQV" );
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_SUBSTITUTION_TAG ],
            &tab->cmn.SubstitutionTag, buffer, spot->offset, spot->NumEvent,
            SUBSTITUTION_TAG_BITSIZE, "seq.SubstitutionTag" );
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_PRE_BASE_FRAMES ],
            &tab->PreBaseFrames, buffer, spot->offset, spot->NumEvent,
            PRE_BASE_FRAMES_BITSIZE, "seq.PreBaseFrames" );
    if ( rc == 0 )
        rc = transfer_bits( cursor, col_idx[ seq_tab_WIDTH_IN_FRAMES ],
            &tab->WidthInFrames, buffer, spot->offset, spot->NumEvent,
            WIDTH_IN_FRAMES_BITSIZE, "seq.WidthInFrames" );
    if ( rc == 0 )
    {
        if ( tab->PulseIndex.element_bits == PULSE_INDEX_BITSIZE_16 )
            rc = transfer_bits( cursor, col_idx[ seq_tab_PULSE_INDEX_16 ],
                &tab->PulseIndex, buffer, spot->offset, spot->NumEvent,
                PULSE_INDEX_BITSIZE_16, "seq.PulsIndex16" );
        else
            rc = transfer_bits( cursor, col_idx[ seq_tab_PULSE_INDEX_32 ],
                &tab->PulseIndex, buffer, spot->offset, spot->NumEvent,
                PULSE_INDEX_BITSIZE_32, "seq.PulsIndex32" );
    }
    if ( buffer != NULL )
        free( buffer );
    return rc;
}


static rc_t seq_load_read_desc( VCursor *cursor, const uint32_t *col_idx,
                                region_type_mapping *mapping,
                                zmw_row * spot, BaseCalls *tab )
{
    uint32_t n;
    /* take the spot-descriptors out of the region-table
       and write them... */

    rc_t rc = rgn_load( &tab->rgn, spot->spot_nr, mapping, spot->NumEvent );
    if ( rc != 0 )
        PLOGERR( klogErr, ( klogErr, rc, "rgn_load( spot #$(spotnr) ) failed",
                            "spotnr=%u", spot->spot_nr ) );

    if ( rc == 0 )
    {
        uint32_t filter_value = SRA_READ_FILTER_PASS;
        if ( spot->HoleStatus != SRA_PACBIO_HOLE_SEQUENCING )
        {
            filter_value = SRA_READ_FILTER_CRITERIA;
        }
        rgn_set_filter_value_for_all( &tab->rgn, filter_value );
    }

    if ( rc == 0 )
    {
        /* write the READ_START - vector */
        rc = rgn_start_data( &tab->rgn, &n );


        if ( n > 255 )
            OUTMSG(( "spot #%lu has %u READS\n", spot->spot_nr, n ));


        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "rgn_start_data( $(spotnr) ) failed",
                                "spotnr=%u", spot->spot_nr ) );
        else
            rc = vdb_write_value( cursor, col_idx[ seq_tab_READ_START ],
                                  tab->rgn.data_32, 32, n, "seq.READ_START" );
    }

    /* write the READ_LEN - vector */
    if ( rc == 0 )
    {
        rc = rgn_len_data( &tab->rgn, &n );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "rgn_len_data( $(spotnr) ) failed",
                                "spotnr=%u", spot->spot_nr ) );
        else
            rc = vdb_write_value( cursor, col_idx[ seq_tab_READ_LEN ],
                                  tab->rgn.data_32, 32, n, "seq.READ_LEN" );
    }

    /* write the READ_TYPE - vector */
    if ( rc == 0 )
    {
        rc = rgn_type_data( &tab->rgn, &n );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "rgn_type_data( $(spotnr) ) failed",
                                "spotnr=%u", spot->spot_nr ) );
        else
            rc = vdb_write_value( cursor, col_idx[ seq_tab_READ_TYPE ],
                                  tab->rgn.data_8, 8, n, "seq.READ_TYPE" );

    }

    /* write the READ_FILTER - vector */
    if ( rc == 0 )
    {
        rc = rgn_filter_data( &tab->rgn, &n );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "rgn_filter_data( $(spotnr) ) failed",
                                "spotnr=%u", spot->spot_nr ) );
        else
            rc = vdb_write_value( cursor, col_idx[ seq_tab_READ_FILTER ],
                                  tab->rgn.data_8, 8, n, "seq.READ_FILTER" );
    }


    /* write the LABEL ( a constant string defined in pl-regions.h" ) */
    if ( rc == 0 )
    {
        rc = vdb_write_value( cursor, col_idx[ seq_tab_LABEL ],
                              (void*)def_label, 8, def_label_len, "seq.LABEL" );
    }

    /* write the LABEL_START ( index into LABEL for every read ) */
    if ( rc == 0 )
    {
        rc = rgn_label_start_data( &tab->rgn, &n );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "rgn_label_start_data( $(spotnr) ) failed",
                                "spotnr=%u", spot->spot_nr ) );
        else
            rc = vdb_write_value( cursor, col_idx[ seq_tab_LABEL_START ],
                                  tab->rgn.data_32, 32, n, "seq.LABEL_START" );
    }

    /* write the LABEL_LEN ( index into LABEL for every read ) */
    if ( rc == 0 )
    {
        rc = rgn_label_len_data( &tab->rgn, &n );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "rgn_label_len_data( $(spotnr) ) failed",
                                "spotnr=%u", spot->spot_nr ) );
        else
            rc = vdb_write_value( cursor, col_idx[ seq_tab_LABEL_LEN ],
                                  tab->rgn.data_32, 32, n, "seq.LABEL_LEN" );
    }

    /* write how many regions(pacbio)/reads(ncbi) this spot has */
    if ( rc == 0 )
    {
        if ( n > 255 )
            n = 255;
        rc = vdb_write_uint8( cursor, col_idx[ seq_tab_NREADS ],
                              (uint8_t)n, "seq.NREADS" );
    }

    /* write hq-start/hq-end into CLIP_QUALITY_LEFT/RIGHT */
    if ( rc == 0 )
        rc = vdb_write_uint32( cursor, col_idx[ seq_tab_CLIP_QUALITY_LEFT ],
                               tab->rgn.hq_rgn.start, "seq.CLIP_QUALITY_LEFT" );

    if ( rc == 0 )
        rc = vdb_write_uint32( cursor, col_idx[ seq_tab_CLIP_QUALITY_RIGHT ],
                               tab->rgn.hq_rgn.end, "seq.CLIP_QUALITY_RIGHT" );

    return rc;
}


static rc_t seq_load_one_spot( VCursor *cursor, const uint32_t *col_idx,
                               zmw_row * spot, BaseCalls *tab )
{
    rc_t rc = vdb_write_uint32( cursor, col_idx[ seq_tab_READ_START ],
                                0, "seq.READ_START" );
    if ( rc == 0 )
        rc = vdb_write_uint32( cursor, col_idx[ seq_tab_READ_LEN ],
                               spot->NumEvent, "seq.READ_LEN" );
    if ( rc == 0 )
        rc = vdb_write_uint8( cursor, col_idx[ seq_tab_READ_TYPE ],
                              SRA_READ_TYPE_BIOLOGICAL, "seq.READ_TYPE" );
    if ( rc == 0 )
        rc = vdb_write_uint8( cursor, col_idx[ seq_tab_READ_FILTER ],
                              SRA_READ_FILTER_PASS, "seq.READ_FILTER" );
    /* write the LABEL ( a constant string defined in pl-regions.h" ) */
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_LABEL ],
                              (void*)def_label, 8, def_label_len, "seq.LABEL" );
    if ( rc == 0 )
        rc = vdb_write_uint32( cursor, col_idx[ seq_tab_LABEL_START ],
                               label_insert_start, "seq.LABEL_START" );
    if ( rc == 0 )
        rc = vdb_write_uint32( cursor, col_idx[ seq_tab_LABEL_LEN ],
                               label_insert_len, "seq.LABEL_LEN" );
    if ( rc == 0 )
        rc = vdb_write_uint8( cursor, col_idx[ seq_tab_NREADS ],
                              1, "seq.NREADS" );
    if ( rc == 0 )
        rc = vdb_write_uint32( cursor, col_idx[ seq_tab_CLIP_QUALITY_LEFT ],
                               1, "seq.CLIP_QUALITY_LEFT" );
    if ( rc == 0 )
        rc = vdb_write_uint32( cursor, col_idx[ seq_tab_CLIP_QUALITY_RIGHT ],
                               spot->NumEvent -1, "seq.CLIP_QUALITY_RIGHT" );
    return rc;
}


static rc_t seq_load_spot( VCursor *cursor, const uint32_t *col_idx,
                           region_type_mapping *mapping, zmw_row * spot, 
                           void * data )
{
    BaseCalls *tab = (BaseCalls *)data;
    rc_t rc = VCursorOpenRow( cursor );
    if ( rc != 0 )
        PLOGERR( klogErr, ( klogErr, rc, "cannot open seq-row on spot# $(spotnr)",
                            "spotnr=%u", spot->spot_nr ) );

    if ( rc == 0 )
        rc = vdb_write_uint32( cursor, col_idx[ seq_tab_HOLE_NUMBER ],
                               spot->HoleNumber, "seq.HOLE_NUMBER" );
    if ( rc == 0 )
        rc = vdb_write_uint8( cursor, col_idx[ seq_tab_HOLE_STATUS ],
                              spot->HoleStatus, "seq.HOLE_STATUS" );
    if ( rc == 0 )
        rc = vdb_write_value( cursor, col_idx[ seq_tab_HOLE_XY ],
                              &spot->HoleXY, HOLE_XY_BITSIZE, 2, "seq.HOLE_XY" );

    if ( rc == 0 )
    {
        /* we load the bases / quality-values and other data belonging
           to this hole(bacbio)/spot(ncbi) */
        if ( spot->NumEvent > 0 )
            rc = seq_load_spot_bases( cursor, tab, col_idx, spot );
        else
            rc = seq_load_zero_bases( cursor, tab, col_idx );

        /* we try to divide the spot into regions(pacbio)/reads(ncbi) */
        if ( rc == 0 )
        {
            if ( mapping != NULL )
                rc = seq_load_read_desc( cursor, col_idx, mapping, spot, tab );
            else
                rc = seq_load_one_spot( cursor, col_idx, spot, tab );
        }
    }

    if ( rc == 0 )
    {
        rc = VCursorCommitRow( cursor );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "cannot commit seq-row on spot# $(spotnr)",
                                "spotnr=%u", spot->spot_nr ) );
    }

    if ( rc == 0 )
    {
        rc = VCursorCloseRow( cursor );
        if ( rc != 0 )
            PLOGERR( klogErr, ( klogErr, rc, "cannot close seq-row on spot# $(spotnr)",
                                "spotnr=%u", spot->spot_nr ) );
    }
    return rc;
}


static void seq_load_info( regions_stat * stat, const char * dst_path,
                           const uint64_t total_bases, const uint64_t total_spots )
{
    const char* accession;
    KLogLevel tmp_lvl = KLogLevelGet();
    KLogLevelSet( klogInfo );

    if ( stat->expands_a > 0 )
        PLOGMSG( klogInfo, ( klogInfo,
             "adapter expanded: $(times) ( in $(spots) spots )",
             "times=%u,spots=%u",
             stat->expands_a, stat->expands_spots ));
    if ( stat->expands_i > 0 )
        PLOGMSG( klogInfo, ( klogInfo,
             "insert expanded : $(times) ( in $(spots) spots )",
             "times=%u,spots=%u",
              stat->expands_i, stat->expands_spots ));
    if ( stat->inserts > 0 )
        PLOGMSG( klogInfo, ( klogInfo,
             "regions inserted: $(times) ( in $(spots) spots )",
             "times=%u,spots=%u",
             stat->inserts, stat->inserts_spots ));
    if ( stat->end_gap > 0 )
        PLOGMSG( klogInfo, ( klogInfo,
             "end-gap inserted: $(times)",
             "times=%u", stat->end_gap ));
    if ( stat->overlapps > 0 )
        PLOGMSG( klogInfo, ( klogInfo,
             "overlapping rngs: $(times)",
             "times=%u", stat->overlapps ));
    if ( stat->removed > 0 )
        PLOGMSG( klogInfo, ( klogInfo,
             "removed rgns    : $(times)",
             "times=%u", stat->removed ));

    accession = strrchr( dst_path, '/' );
    if( accession == NULL )
        accession = dst_path;
    else
        accession++;

    PLOGMSG( klogInfo, ( klogInfo, "loaded",
            "severity=total,status=success,accession=%s,spot_count=%lu,base_count=%lu,bad_spots=0",
             accession, total_spots, total_bases ));

    KLogLevelSet( tmp_lvl );
}


static rc_t seq_loader( ld_context *lctx, const char * table_name,
                        bool cache_content )
{
    BaseCalls BaseCallsTab;
    bool rgn_present;

    /* opens all hdf5-tables, which are needed to load the sequence-table */
    rc_t rc = open_BaseCalls( lctx->hdf5_dir, &BaseCallsTab,
                              "PulseData/BaseCalls", cache_content, &rgn_present );
    if ( rc == 0 )
    {
        /* calculates the total number of bases, according to the zmw-table */
        uint64_t total_bases = zmw_total( &BaseCallsTab.cmn.zmw );
        /* calculates the total number of spots, according to the zmw-table */
        uint64_t total_spots = BaseCallsTab.cmn.zmw.NumEvent.extents[ 0 ];

        KLogLevel tmp_lvl = KLogLevelGet();
        KLogLevelSet( klogInfo );
        PLOGMSG( klogInfo, ( klogInfo,
                 "loading sequence-table ( $(bases) bases / $(spots) spots ):",
                 "bases=%lu,spots=%lu", total_bases, total_spots ));
        KLogLevelSet( tmp_lvl );

        /* checks that all tables, which are loaded do have the correct
           number of values (the number that the zmw-table requests) */
        if ( check_BaseCall_totalcount( &BaseCallsTab, total_bases ) )
        {
            region_type_mapping mapping;

            if ( rgn_present )
            {
                const KNamelist *region_types;
                /* read the meta-data-entry "RegionTypes" of the hdf5-regions-table
                   into a KNamelist */
                rc = KArrayFileGetMeta ( BaseCallsTab.rgn.hdf5_regions.af, 
                                         "RegionTypes", &region_types );
                if ( rc != 0 )
                {
                    LOGERR( klogErr, rc, "cannot read Regions.RegionTypes" );
                }
                else
                {
                    /* extract the region-type-mapping out of the read KNamelist */
                    rc = rgn_extract_type_mappings( region_types, &mapping );
                    KNamelistRelease ( region_types );
                    if ( rc != 0 )
                    {
                        LOGERR( klogErr, rc, "cannot map regions-types" );
                    }
                }
            }

            if ( rc == 0 )
            {
                /* holds the vdb-column-index for every column we write */
                uint32_t col_idx[ seq_tab_count ];
                int32_t to_exclude;

                /* depending on the bit-size of the PulseIndex-table in HDF5
                   exclude the opposite column from VDB */
                if ( BaseCallsTab.PulseIndex.element_bits == PULSE_INDEX_BITSIZE_16 )
                    to_exclude = seq_tab_PULSE_INDEX_32;
                else
                    to_exclude = seq_tab_PULSE_INDEX_16;

                /* add all columns to the vdb-cursor */
                rc = add_columns( lctx->cursor, seq_tab_count, to_exclude,
                                  col_idx, seq_tab_names );
                if ( rc == 0 )
                {
                    rc = VCursorOpen( lctx->cursor );
                    if ( rc != 0 )
                        LOGERR( klogErr, rc, "cannot open cursor on seq-table" );
                    else
                    {
                        const uint8_t platform = SRA_PLATFORM_PACBIO_SMRT;

                        rc = VCursorDefault ( lctx->cursor, col_idx[ seq_tab_PLATFORM ],
                                              8, &platform, 0, 1 );
                        if ( rc != 0 )
                            LOGERR( klogErr, rc, "cannot set cursor-default on seq-table for platform-column" );
                        else
                        {
                            region_type_mapping *mapping_ptr = NULL;
        
                            if ( rgn_present )
                            {
                                mapping_ptr = &mapping;
                            }
                            /* call for every spot the function >seq_load_spot< */
                            rc = zmw_for_each( &BaseCallsTab.cmn.zmw, lctx, col_idx,
                                               mapping_ptr, false, seq_load_spot, &BaseCallsTab );
                        }
                    }
                }
            }

            if ( rgn_present )
            {
                seq_load_info( &BaseCallsTab.rgn.stat, lctx->dst_path, total_bases, total_spots );
            }
        }   
        else
            rc = RC( rcExe, rcNoTarg, rcAllocating, rcParam, rcInvalid );
        close_BaseCalls( &BaseCallsTab );
    }

    return rc;
}


/* HDF5-Groups and tables used to load the PULSE-table */
static const char * seq_groups_to_check[] = 
{ 
    "PulseData",
    "PulseData/BaseCalls",
    "PulseData/BaseCalls/ZMW",
    NULL
};

static const char * seq_tables_to_check[] = 
{ 
    "PulseData/BaseCalls/Basecall",
    "PulseData/BaseCalls/DeletionQV",
    "PulseData/BaseCalls/DeletionTag",
    "PulseData/BaseCalls/InsertionQV",
    "PulseData/BaseCalls/PreBaseFrames",
    "PulseData/BaseCalls/PulseIndex",
    "PulseData/BaseCalls/QualityValue",
    "PulseData/BaseCalls/SubstitutionQV",
    "PulseData/BaseCalls/SubstitutionTag",
    "PulseData/BaseCalls/WidthInFrames",
    "PulseData/BaseCalls/ZMW/HoleNumber",
    "PulseData/BaseCalls/ZMW/HoleStatus",
    "PulseData/BaseCalls/ZMW/HoleXY",
    "PulseData/BaseCalls/ZMW/NumEvent",
    NULL
};


static const char * seq_schema_template = "SEQUENCE";
static const char * seq_table_to_create = "SEQUENCE";


rc_t load_seq( ld_context *lctx, bool cache_content, bool check_src_obj )
{
    rc_t rc = 0;
    if ( check_src_obj )
        rc = check_src_objects( lctx->hdf5_dir, seq_groups_to_check, 
                                seq_tables_to_check, true );
    if ( rc == 0 )
        rc = load_table( lctx, seq_schema_template, 
                         seq_table_to_create, seq_loader,
                         cache_content );
    return rc;
}
