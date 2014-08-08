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
#include <klib/log.h>

#include "support.h"
#include "t_al_iter.h"
#include "options.h"

#include <stdlib.h>
#include <string.h>

#include <common/test_assert.h>


typedef struct ext_rec ext_rec;
struct ext_rec
{
    /* orientation towards reference ( false...in ref-orientation / true...reverse) */
    bool reverse;
    /* ptr to quality... */
    uint8_t * quality;
};


static void handle_base_pos( ReferenceIterator *ref_iter,
                             const PlacementRecord *rec,
                             uint8_t * qual,
                             bool nodebug )
{
    INSDC_coord_zero seq_pos;
    int32_t state = ReferenceIteratorState ( ref_iter, &seq_pos );
    ext_rec *xrec = ( ext_rec * ) PlacementRecordCast ( rec, placementRecordExtension1 );
    bool reverse = xrec->reverse;

    if ( !nodebug )
        OUTMSG(( "[%lu.%u ", rec->id, seq_pos ));

    *qual = xrec->quality[ seq_pos ];
    if ( ( state & align_iter_first ) == align_iter_first )
    {
        char mapq = ( rec->mapq + 33 );
        OUTMSG(( "^%c", mapq ));
    }

    if ( ( state & align_iter_last ) == align_iter_last )
        OUTMSG(( "$" ));

    if ( ( state & align_iter_insert ) == align_iter_insert )
    {
        const INSDC_4na_bin *bases;
        uint32_t n = ReferenceIteratorBasesInserted ( ref_iter, &bases );
        char * s = dup_2_ascii( bases, n, reverse );
        OUTMSG(( "+%u%s", n, s ));
        free( s );
    }

    if ( ( state & align_iter_delete ) == align_iter_delete )
    {
        const INSDC_4na_bin *bases;
        INSDC_coord_zero ref_pos;
        uint32_t n = ReferenceIteratorBasesDeleted ( ref_iter, &ref_pos, &bases );
        if ( bases != NULL )
        {
            char * s = dup_2_ascii( bases, n, reverse );
            OUTMSG(( "-%u%s", n, s ));
            free( s );
            free( (void *) bases );
        }
        else
        {
            OUTMSG(( "-%u?", n ));
        }
    }

    if ( ( state & align_iter_skip ) == align_iter_skip )
        OUTMSG(( "*" ));
    else
    {
        if ( ( state & align_iter_match ) == align_iter_match )
            OUTMSG(( "%c", reverse ? ',' : '.' ));
        else
            OUTMSG(( "%c", _4na_to_ascii( state & 0x0F, reverse ) ));
    }

    if ( !nodebug )
        OUTMSG(( "]" ));
}


static rc_t walk_ref_iter_position( ReferenceIterator *ref_iter,
                                    struct ReferenceObj const * refobj,
                                    bool skip_empty,
                                    bool nodebug )
{
    INSDC_coord_zero pos;
    uint32_t depth;
    INSDC_4na_bin base;

    rc_t rc = ReferenceIteratorPosition ( ref_iter, &pos, &depth, &base );
    if ( rc != 0 )
    {
        if ( GetRCState( rc ) != rcDone )
            LOGERR( klogInt, rc, "ReferenceIteratorNextPos() failed" );
    }
    else
    {
        rc_t rc1 = 0;
        const char * reference_name = NULL;
        char c = _4na_to_ascii( base, false );
        rc = ReferenceObj_SeqId( refobj, &reference_name );
        if ( rc == 0 )
        {
            OUTMSG(( "%s\t%u\t%c\t%u", reference_name, pos, c, depth ));
            if ( depth > 0 )
            {
                const PlacementRecord *rec;
                rc1 = ReferenceIteratorNextPlacement ( ref_iter, &rec );
                if ( rc1 == 0 )
                {
                    uint8_t qualities[ 4096 ];
                    uint32_t i = 0;
                    OUTMSG(( "\t" ));
                    while ( rc1 == 0 )
                    {
                        handle_base_pos( ref_iter, rec, &( qualities[ i++ ] ), nodebug );
                        rc1 = ReferenceIteratorNextPlacement ( ref_iter, &rec );
                    }
                    OUTMSG(( "\t" ));
                    for ( i = 0; i < depth; ++i )
                    {
                        char c = ( qualities[ i ] + 33 );
                        OUTMSG(( "%c", c ));
                    }
                }
            }
            OUTMSG(( "\n" ));
        }
        if ( GetRCState( rc1 ) == rcDone ) rc1 = 0;
        rc = rc1;
    } 
    return rc;
}


static rc_t walk_ref_iter_window( ReferenceIterator *ref_iter,
                                  struct ReferenceObj const * refobj,
                                  bool skip_empty,
                                  bool nodebug )
{
    rc_t rc = 0;
    while ( rc == 0 )
    {
        rc = ReferenceIteratorNextPos ( ref_iter, skip_empty );
        if ( rc != 0 )
        {
            if ( GetRCState( rc ) != rcDone )
                LOGERR( klogInt, rc, "ReferenceIteratorNextPos() failed" );
        }
        else
        {
            rc = walk_ref_iter_position( ref_iter, refobj, skip_empty, nodebug );
        }
    }
    if ( GetRCState( rc ) == rcDone ) rc = 0;
    return rc;
}


static rc_t walk_ref_iter_reference( ReferenceIterator *ref_iter,
                                     struct ReferenceObj const * refobj,
                                     bool skip_empty,
                                     bool nodebug )
{
    rc_t rc = 0;
    while ( rc == 0 )
    {
        INSDC_coord_zero first_pos;
        INSDC_coord_len len;
        rc = ReferenceIteratorNextWindow ( ref_iter, &first_pos, &len );
        if ( rc != 0 )
        {
            if ( GetRCState( rc ) != rcDone )
                LOGERR( klogInt, rc, "ReferenceIteratorNextWindow() failed" );
        }
        else
        {
            OUTMSG(( "walking ref-iter on window: ( from %u to %u )\n",
                     first_pos, first_pos + len - 1 ));
            rc = walk_ref_iter_window( ref_iter, refobj, skip_empty, nodebug );
        }
    }
    if ( GetRCState( rc ) == rcDone ) rc = 0;
    return rc;
}


static rc_t walk_ref_iter( ReferenceIterator *ref_iter,
                           bool skip_empty,
                           bool nodebug )
{
    rc_t rc = 0;
    while( rc == 0 )
    {
        struct ReferenceObj const * refobj;
        INSDC_coord_zero first_pos;
        INSDC_coord_len len;
        rc = ReferenceIteratorNextReference( ref_iter, &first_pos, &len, &refobj );
        if ( rc == 0 && refobj != NULL )
        {
            const char *name;
            rc = ReferenceObj_SeqId( refobj, &name );
            if ( rc != 0 )
                LOGERR( klogInt, rc, "ReferenceObj_SeqId() failed" );
            else
            {
                OUTMSG(( "walking ref-iter on reference: >%s< ( from %u to %u )\n",
                         name, first_pos, first_pos + len - 1 ));
                rc = walk_ref_iter_reference( ref_iter, refobj, skip_empty, nodebug );
            }
        }
    }
    if ( GetRCState( rc ) == rcDone ) rc = 0;
    return rc;
}


static rc_t get_prim_align_cursor( const VDatabase *db, const VCursor **curs )
{
    const VTable *tbl;
    rc_t rc = VDatabaseOpenTableRead ( db, &tbl, "PRIMARY_ALIGNMENT" );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "VDatabaseOpenTableRead(PRIMARY_ALIGNMENT) failed" );
    else
    {
        rc = VTableCreateCursorRead ( tbl, curs );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "VTableCreateCursorRead(tab=PRIMARY_ALIGNMENT) failed" );
        else
        {
            uint32_t throw_away_idx;
            rc = VCursorAddColumn ( *curs, &throw_away_idx, "QUALITY" );
            if ( rc != 0 )
                LOGERR( klogInt, rc, "VTableCreateCursorRead(QUALITY) failed" );
            else
            {
                rc = VCursorAddColumn ( *curs, &throw_away_idx, "REF_ORIENTATION" );
                if ( rc != 0 )
                    LOGERR( klogInt, rc, "VTableCreateCursorRead(REF_ORIENTATION) failed" );
            }
        }
        VTableRelease ( tbl );
    }
    return rc;
}


static rc_t read_base_and_len( struct VCursor const *curs,
                               const char * name,
                               int64_t row_id,
                               const void ** base,
                               uint32_t * len )
{
    uint32_t column_idx;
    rc_t rc = VCursorGetColumnIdx ( curs, &column_idx, name );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "VCursorGetColumnIdx() failed" );
    else
    {
        uint32_t elem_bits, boff, len_intern;
        const void * ptr;
        rc = VCursorCellDataDirect ( curs, row_id, column_idx, 
                                     &elem_bits, &ptr, &boff, &len_intern );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "VCursorCellDataDirect() failed" );
        else
        {
            if ( len != NULL ) *len = len_intern;
            if ( base != NULL ) *base = ptr;
        }
    }
    return rc;
}


static rc_t CC ext_rec_populate ( void *obj,
    const PlacementRecord *placement, struct VCursor const *curs,
    INSDC_coord_zero ref_window_start, INSDC_coord_len ref_window_len,
    void *data )
{
    const bool * orientation;
    rc_t rc = read_base_and_len( curs, "REF_ORIENTATION", placement->id, (const void **)&orientation, NULL );
    if ( rc == 0 )
    {
        uint8_t * quality;
        uint32_t quality_len;
        ext_rec * rec = ( ext_rec * ) obj;

        rec->reverse = *orientation;
        rc = read_base_and_len( curs, "QUALITY", placement->id, (const void **)&quality, &quality_len );
        if ( rc == 0 )
        {
            rec->quality = ( uint8_t * )rec;
            rec->quality += sizeof ( * rec );
            memcpy( rec->quality, quality, quality_len );
        }
    }
    return rc;
}


static size_t CC ext_rec_size ( struct VCursor const *curs, int64_t row_id, void *data )
{
    size_t res = 0;
    uint32_t q_len;
    rc_t rc = read_base_and_len( curs, "QUALITY", row_id, NULL, &q_len );
    if ( rc == 0 )
    {
        ext_rec * rec;
        res = ( sizeof *rec ) + q_len;
    }
    return res;
}


static rc_t test_ref_iterator( trans_ctx *ctx,
                               const char * ref_name, 
                               INSDC_coord_zero ref_pos,
                               INSDC_coord_len ref_len,
                               bool skip_empty,
                               bool nodebug )
{
    rc_t rc;
    ReferenceIterator *ref_iter = NULL;
    struct VCursor const *ref_cursor = NULL;
    struct VCursor const *align_cursor = NULL;
    PlacementRecordExtendFuncs ef;

    /* (1) make the reference-iterator */
    ef.data = ( void * )ctx->almgr;
    ef.destroy = NULL; /* ext_rec_destroy; */
    ef.populate = ext_rec_populate;
    ef.alloc_size = ext_rec_size;
    ef.fixed_size = 0;

    rc = AlignMgrMakeReferenceIterator ( ctx->almgr, &ref_iter, &ef, 0 /* min_mapq*/ );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "AlignMgrMakeReferenceIterator() failed" );

    /* (2) prepare the alignmnet-cursor (for the needs of the callback...)*/
    if ( rc == 0 )
        rc = get_prim_align_cursor( ctx->db, &align_cursor );

    /* (3) all placements to the ref-iter AND WALK THE REFERENCE-ITERATOR! */
    if ( rc == 0 )
    {
        rc = ReferenceIteratorAddPlacements( ref_iter, ctx->ref_obj, ref_pos, ref_len,
                                             ref_cursor, align_cursor, primary_align_ids, NULL );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "ReferenceIteratorAddPlacements() failed" );
        else
        {
            /* =============================================== */
            rc = walk_ref_iter( ref_iter, skip_empty, nodebug );
            /* =============================================== */
        }
    }

    if ( ref_cursor != NULL ) VCursorRelease ( ref_cursor );
    if ( align_cursor != NULL ) VCursorRelease ( align_cursor );
    if ( ref_iter != NULL ) ReferenceIteratorRelease( ref_iter );
    return rc;
}


/* =============================================================================================== */
rc_t test_ref_list( Args * args )
{
    uint32_t nodebug, skipempty;
    trans_opt opt;
    trans_ctx ctx;

    rc_t rc = make_trans_opt( &opt, args );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "make_trans_opt() failed" );

    if ( rc == 0 )
        rc = get_uint32_option( args, OPTION_NDBG, &nodebug, 0 );
    if ( rc == 0 )
        rc = get_uint32_option( args, OPTION_SKE, &skipempty, 1 );

    if ( rc == 0 )
    {
        rc = make_trans_ctx( &ctx, &opt, true );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "make_trans_ctx() failed" );
    }

    if ( rc == 0 )
    {
        if ( nodebug == 0 )
        {
            OUTMSG(( "testing ref_iterator on >%s<\n", opt.fname ));
        }
        /* ============================================================================= */
        rc = test_ref_iterator( &ctx, opt.ref_name, opt.ref_offset[0], opt.ref_len[0],
                                ( skipempty != 0 ), ( nodebug != 0 ) );
        /* ============================================================================= */
    }

    free_trans_ctx( &ctx );
    return rc;
}
