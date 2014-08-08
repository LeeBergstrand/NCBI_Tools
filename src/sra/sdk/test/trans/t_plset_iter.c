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
#include "t_pl_iter.h"
#include "options.h"

#include <stdlib.h>
#include <string.h>

#include <common/test_assert.h>


static rc_t placement_set_iter_records_at( PlacementSetIterator *pl_set_iter, INSDC_coord_zero pos )
{
    rc_t rc = 0;
    while ( rc == 0 )
    {
        const PlacementRecord *rec;
        rc = PlacementSetIteratorNextRecordAt ( pl_set_iter, pos, &rec );
        if ( rc != 0 )
        {
            if ( GetRCState( rc ) != rcDone )
                LOGERR( klogInt, rc, "PlacementSetIteratorNextRecordAt() failed" );
        }
        else
        {
            OUTMSG(( "at [%u] record #%u ( %u ... %u )\n", 
                      pos, rec->id, rec->pos, rec->pos + rec->len - 1 ));
        }
    }
    if ( GetRCState( rc ) == rcDone ) rc = 0;
    return rc;
}


static rc_t walk_placement_set_iter_by_pos( PlacementSetIterator *pl_set_iter )
{
    rc_t rc = 0;

    OUTMSG(( "walking the Placement-Set-Iterator positions!\n" ));
    while ( rc == 0 )
    {
        INSDC_coord_len len;
        INSDC_coord_zero pos;
        rc = PlacementSetIteratorNextAvailPos ( pl_set_iter, &pos, &len );
        if ( rc != 0 )
        {
            if ( GetRCState( rc ) != rcDone )
                LOGERR( klogInt, rc, "PlacementSetIteratorNextAvailPos() failed" );
        }
        else
        {
            OUTMSG(( "PlacementSetIteratorNextAvailPos() -> pos=%u, len=%u\n",
                     pos, len ));
            rc = placement_set_iter_records_at( pl_set_iter, pos );
        }
    }
    if ( GetRCState( rc ) == rcDone ) rc = 0;
    return rc;
}


static rc_t walk_placement_set_iter_by_window( PlacementSetIterator *pl_set_iter )
{
    rc_t rc = 0;
    while ( rc == 0 )
    {
        INSDC_coord_zero w_pos;
        INSDC_coord_len w_len;
        rc = PlacementSetIteratorNextWindow ( pl_set_iter, &w_pos, &w_len );
        if ( rc != 0 )
        {
            if ( GetRCState( rc ) != rcDone )
                LOGERR( klogInt, rc, "PlacementSetIteratorNextWindow() failed" );
        }
        else
        {
            OUTMSG(( "walking WINDOW: %u ... %u\n", w_pos, w_pos + w_len - 1 ));
            rc = walk_placement_set_iter_by_pos( pl_set_iter );
        }
    }
    if ( GetRCState( rc ) == rcDone ) rc = 0;
    return rc;
}


static rc_t walk_placement_set_iter_by_ref( PlacementSetIterator *pl_set_iter )
{
    rc_t rc = 0;
    OUTMSG(( "walking placement-set-iterator\n" ));
    while ( rc == 0 )
    {
        struct ReferenceObj const *refobj;
        INSDC_coord_zero first_pos;
        INSDC_coord_len len;
        rc = PlacementSetIteratorNextReference ( pl_set_iter, &first_pos, &len, &refobj );
        if ( rc != 0 || refobj == NULL )
        {
            if ( GetRCState( rc ) != rcDone )
            LOGERR( klogInt, rc, "PlacementSetIteratorNextReference() failed" );
        }
        else
        {
            const char *name;
            rc = ReferenceObj_SeqId( refobj, &name );
            if ( rc != 0 )
                LOGERR( klogInt, rc, "ReferenceObj_Name() failed" );
            else
            {
                OUTMSG(( "walking REF: >%s< ( outer window: %u - %u )\n",
                         name, first_pos, first_pos + len - 1 ));
                rc = walk_placement_set_iter_by_window( pl_set_iter );
            }
        }
    }
    if ( GetRCState( rc ) == rcDone ) rc = 0;
    return rc;
}


rc_t test_placement_set_iter( Args * args )
{
    trans_opt opt;
    trans_ctx ctx;

    rc_t rc = make_trans_opt( &opt, args );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "make_trans_opt() failed" );

    OUTMSG(( "testing placement-set-iterator on >%s<\n", opt.fname ));
    if ( rc == 0 )
    {
        rc = make_trans_ctx( &ctx, &opt, true );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "make_trans_ctx() failed" );
    }
    if ( rc == 0 )
    {
        PlacementSetIterator *pl_set_iter;
        rc = AlignMgrMakePlacementSetIterator ( ctx.almgr, &pl_set_iter );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "AlignMgrMakePlacementSetIterator() failed" );
        else
        {
            uint32_t i;
            for ( i = 0; i < opt.count && rc == 0; ++i )
            {
                PlacementIterator *pl_iter;
                rc = make_pl_iter( &pl_iter, i, &opt, &ctx );
                if ( rc == 0 )
                {
                    rc = PlacementSetIteratorAddPlacementIterator( pl_set_iter, pl_iter );
                    if ( rc != 0 )
                        LOGERR( klogInt, rc, "PlacementSetIteratorAddPlacementIterator() failed" );
                }
            }
            if ( rc == 0 )
                walk_placement_set_iter_by_ref( pl_set_iter );
            PlacementSetIteratorRelease( pl_set_iter );
        }
    }

    free_trans_ctx( &ctx );
    return rc;
}
