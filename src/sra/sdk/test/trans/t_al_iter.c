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

#include <kapp/args.h>
#include <klib/out.h>
#include <klib/text.h>
#include <align/iterator.h>
#include <align/manager.h>
#include "support.h"
#include <stdlib.h>

#include "options.h"

#include <common/test_assert.h>

static void walk_alignment_iter( AlignmentIterator *it )
{
    do
    {
        char c;
        int32_t pos;
        INSDC_coord_zero seq_pos;
        uint32_t state = AlignmentIteratorState ( it, &seq_pos );
        AlignmentIteratorPosition ( it, &pos );

        OUTMSG(( "[%d][%u] ", pos, seq_pos ));

        if ( ( state & align_iter_first ) == align_iter_first )
        {
            OUTMSG(( "<first>" ));
        }
        if ( ( state & align_iter_last ) == align_iter_last )
        {
            OUTMSG(( "<last>" ));
        }

        c = _4na_to_ascii( state & 0xF, false );

        if ( ( state & align_iter_insert ) == align_iter_insert )
        {
            const INSDC_4na_bin *bases;

            uint32_t n = AlignmentIteratorBasesInserted ( it, &bases );
            char * s = dup_2_ascii( bases, n, true );

            if ( ( state & align_iter_match ) == align_iter_match )
                OUTMSG(( "MATCH<%c>", c ));
            else
                OUTMSG(( "MISMATCH<%c>", c ));

            OUTMSG(( "<INS:%u:%.*s>\n", n, n, s ));
            free( s );
        }
        else if ( ( state & align_iter_delete ) == align_iter_delete )
        {
            INSDC_coord_zero at;
            uint32_t n = AlignmentIteratorBasesDeleted( it, &at );
            if ( ( state & align_iter_match ) == align_iter_match )
                OUTMSG(( "MATCH<%c>", c ));
            else
                OUTMSG(( "MISMATCH<%c>", c ));
            OUTMSG(( "<DEL:%u:at %d>\n", n, at ));
        }
        else if ( ( state & align_iter_skip ) == align_iter_skip )
        {
            OUTMSG(( "SKIP\n" ));
        }
        else
        {
            if ( ( state & align_iter_match ) == align_iter_match )
                OUTMSG(( "MATCH<%c>\n", c ));
            else
                OUTMSG(( "MISMATCH<%c>\n", c ));
        }

    } while( AlignmentIteratorNext ( it ) == 0 );
}


static rc_t make_al_iter( AlignmentIterator **it,
                          const char *HMM, const char *HRO, const INSDC_4na_bin *READ,
                          const int32_t * RO, uint32_t ro_count, int32_t ref_pos )
{
    rc_t rc = 0;
    size_t read_len = string_size ( HMM );
    size_t hro_len  = string_size ( HRO );
    if ( read_len != hro_len )
    {
        rc = RC( rcApp, rcNoTarg, rcConstructing, rcItem, rcInvalid );
        OUTMSG(( "difference in len of HAS_MISMATCH/HAS_REFOFFSET/READ!\n" ));
    }
    else
    {
        const struct AlignMgr * alm;
        bool * b_hmm = make_bool_array( HMM );
        bool * b_hro = make_bool_array( HRO );
        uint32_t rofs_len = count_true( b_hro, read_len );
        uint32_t ref_len  = read_len + vector_sum( RO, rofs_len, false );

        OUTMSG(( "read_len = %u\n", read_len ));
        OUTMSG(( "ref_len  = %u\n\n", ref_len ));

        rc = AlignMgrMakeRead ( &alm );
        if ( rc == 0 )
        {
            rc = AlignMgrMakeAlignmentIterator (
                    alm,        /* the alignment-manager */
                    it,         /* the iterator to be created */
                    false,      /* make a copy of the given data */
                    ref_pos,    /* where the alignment starts, absolute on reference */
                    ref_len,    /* how long the alignment is, against the reference */
                    READ,       /* 4na of READ */
                    read_len,   /* read-len */
                    b_hmm,      /* has-mismatch (array of bool) */
                    b_hro,      /* has-ref-offset (array of bool) */
                    RO,         /* ref-offset (array of int32 ) */
                    ro_count,   /* how many ref-offset-values in array */
                    ref_pos,    /* where the loading, window starts, for this test == ref_pos */
                    ref_len     /* how long the loading window is, for this test == ref_len */
                    );
            if ( rc != 0 )
                OUTMSG(( "AlignMgrMakeAlignmentIterator = %R\n", rc ));
            AlignMgrRelease ( alm );
        }

        free( b_hmm );
        free( b_hro );
    }
    return rc;
}


rc_t test_al_iter( Args * args )
{
    const char *HMM;
    const char *HRO;
    const char *READ;
    const int32_t *RO;
    uint32_t ro_count;
    int32_t ref_pos;
    AlignmentIterator *it;

    rc_t rc = get_str_option( args, OPTION_HMM, &HMM );
    if ( rc == 0 )
        rc = get_int32_option( args, OPTION_RP, &ref_pos, 1000 );
    if ( rc == 0 )
        rc = get_str_option( args, OPTION_HRO, &HRO );
    if ( rc == 0 )
        rc = get_str_option( args, OPTION_READ, &READ );
    if ( rc == 0 )
        rc = get_ro( args, OPTION_RO, &RO, &ro_count );

    if ( rc == 0 )
    {
        INSDC_4na_bin * bREAD;

        OUTMSG(( "POS : '%d'\n", ref_pos ));
        OUTMSG(( "HMM : '%s'\n", HMM ));
        OUTMSG(( "HRO : '%s'\n", HRO ));
        OUTMSG(( "READ: '%s'\n", READ ));
        print_ro( RO, ro_count );

        bREAD = dup_2_4na( READ );

        rc = make_al_iter( &it, HMM, HRO, bREAD, RO, ro_count, ref_pos );
        if ( rc == 0 )
        {
            walk_alignment_iter( it );
            AlignmentIteratorRelease ( it );
        }

        free( bREAD );
    }
    return rc;
}
