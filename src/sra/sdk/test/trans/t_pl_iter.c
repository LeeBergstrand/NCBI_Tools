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


static rc_t list_records_at( PlacementIterator *pi, INSDC_coord_zero pos )
{
    rc_t rc = 0;
    while ( rc == 0 )
    {
        const PlacementRecord *rec;
        rc = PlacementIteratorNextRecordAt ( pi,  pos,  &rec );
        if ( rc == 0 )
        {
            OUTMSG(( "\tat [%u] record #%u (len=%u)\n",
                     pos, rec->id, rec->len ));
        }
    }
    if ( rc != 0 )
    {
        if ( GetRCState( rc ) == rcDone )
            rc = 0;
        else
            LOGERR( klogInt, rc, "PlacementIteratorNextRecordAt() failed" );
    }
    return rc;
}


static rc_t walk_placement_iterator( PlacementIterator *pi )
{
    rc_t rc = 0;

    OUTMSG(( "walking the Placement-Iterator!\n" ));
    while ( rc == 0 )
    {
        INSDC_coord_len len;
        INSDC_coord_zero pos;

        rc = PlacementIteratorNextAvailPos ( pi, &pos, &len );
        OUTMSG(( "PlacementIteratorNextAvailPos() -> pos=%u, len=%u, end_reached=%c\n",
                 pos, len, ( ( GetRCState( rc ) == rcDone ) ? 'y' : 'n' ) ));
        if ( rc == 0 )
            rc = list_records_at( pi, pos );
    }
    return rc;
}


rc_t make_pl_iter( PlacementIterator **pl_iter, uint32_t idx, trans_opt *opt, trans_ctx *ctx )
{
    rc_t rc;
    int32_t min_mapq = 0;
    struct VCursor const *ref_cur = NULL;
    struct VCursor const *align_cur = NULL;
    align_id_src what_ids = primary_align_ids;
    const PlacementRecordExtendFuncs *ext_0 = NULL;
    const PlacementRecordExtendFuncs *ext_1 = NULL;
    const char * rd_group = NULL;

    OUTMSG(( "creating placement-iterator [%u ... %u]\n", 
              opt->ref_offset[idx], opt->ref_offset[idx] + opt->ref_len[idx] - 1 ));

    rc = ReferenceObj_MakePlacementIterator ( ctx->ref_obj, pl_iter,
            opt->ref_offset[ idx ], opt->ref_len[ idx ],
            min_mapq, ref_cur, align_cur, what_ids, ext_0, ext_1, rd_group );

    if ( rc != 0 )
        LOGERR( klogInt, rc, "ReferenceObj_MakePlacementIterator() failed" );
    return rc;
}


rc_t test_placement_iter( Args * args )
{
    trans_opt opt;
    trans_ctx ctx;

    rc_t rc = make_trans_opt( &opt, args );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "make_trans_opt() failed" );

    OUTMSG(( "testing placement-iterator on >%s<\n", opt.fname ));
    if ( rc == 0 )
    {
        rc = make_trans_ctx( &ctx, &opt, true );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "make_trans_ctx() failed" );
    }

    if ( rc == 0 )
    {
        PlacementIterator *pl_iter;
        rc = make_pl_iter( &pl_iter, 0, &opt, &ctx );
        if ( rc == 0 )
        {
            rc = walk_placement_iterator( pl_iter );
            PlacementIteratorRelease ( pl_iter );
        }
    }
    free_trans_ctx( &ctx );
    return rc;
}
