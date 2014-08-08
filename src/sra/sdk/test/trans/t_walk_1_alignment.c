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
#include <klib/text.h>

#include <vdb/table.h>
#include <vdb/cursor.h>

#include "support.h"
#include "options.h"

#include <os-native.h>
#include <sysalloc.h>

#include <string.h>
#include <stdlib.h>

#include <common/test_assert.h>


typedef struct alignment_data alignment_data;
struct alignment_data
{
    int64_t id;
    char * ref_seq_name;
    INSDC_coord_zero ref_pos;
    INSDC_coord_len ref_len;
    char * read;
    char * has_mismatch;
    char * has_ref_offset;
    int32_t * ref_offset;
    uint32_t ref_offset_len;
};


static void free_alignment_data( alignment_data * data )
{
    free( data->ref_seq_name );
    free( data->read );
    free( data->has_mismatch );
    free( data->has_ref_offset );
    free( data->ref_offset );
}


static rc_t add_columns( const VCursor *curs, const char ** names )
{
    rc_t rc = 0;
    const char ** ptr = names;
    while( rc == 0 && *ptr != NULL )
    {
        uint32_t idx;
        rc = VCursorAddColumn ( curs, &idx, "%s", *ptr );
        ptr++;
    }
    return rc;
}


static const char * columns[] = { "REF_SEQ_ID", "REF_POS", "REF_LEN", "READ",
                                  "HAS_MISMATCH", "HAS_REF_OFFSET", "REF_OFFSET", NULL };


static rc_t read_string_column( const VCursor *curs, const char * name, int64_t row_id, char ** dst )
{
    uint32_t idx;
    rc_t rc = VCursorGetColumnIdx ( curs, &idx, "%s", name );
    if ( rc == 0 )
    {
        uint32_t elem_bits, boff, row_len;
        const void *base;
        rc = VCursorCellDataDirect ( curs, row_id, idx, &elem_bits, &base, &boff, &row_len );
        if ( rc == 0 )
        {
            if ( row_len > 0 )
            {
                *dst = calloc( row_len + 1, 1 );
                if ( *dst != NULL )
                {
                    memcpy( *dst, base, row_len );
                    (*dst)[ row_len ] = 0;
                }
            }
        }
    }
    return rc;
}


static rc_t read_int32_array( const VCursor *curs, const char * name, int64_t row_id, int32_t ** dst, uint32_t *len )
{
    uint32_t idx;
    rc_t rc = VCursorGetColumnIdx ( curs, &idx, "%s", name );
    if ( rc == 0 )
    {
        uint32_t elem_bits, boff;
        const void *base;
        rc = VCursorCellDataDirect ( curs, row_id, idx, &elem_bits, &base, &boff, len );
        if ( rc == 0 )
        {
            if ( *len > 0 )
            {
                size_t size = ( *len ) * ( sizeof ** dst );
                *dst = calloc( size, 1 );
                if ( *dst != NULL )
                    memcpy( *dst, base, size );
            }
        }
    }
    return rc;
}


static rc_t read_column_direct( const VCursor *curs, const char * name, int64_t row_id, const void ** ptr )
{
    uint32_t idx;
    rc_t rc = VCursorGetColumnIdx ( curs, &idx, "%s", name );
    if ( rc == 0 )
    {
        uint32_t elem_bits, boff, row_len;
        rc = VCursorCellDataDirect ( curs, row_id, idx, &elem_bits, ptr, &boff, &row_len );
    }
    return rc;
}


static void print_ref_offset( int32_t * a, uint32_t len )
{
    uint32_t i;
    OUTMSG(( "REF_OFFSET: " ));
    for ( i = 0; i < len; ++i )
        OUTMSG(( "[%d]", a[ i ] ));
    OUTMSG(( "\n" ));
}


static rc_t discover_ref_seq_name( const VDatabase *db, uint32_t table_select, alignment_data * data )
{
    rc_t rc = 0;
    const VTable *tab;

    switch ( table_select )
    {
    case 1 : rc = VDatabaseOpenTableRead ( db, &tab, "%s", "PRIMARY_ALIGNMENT" ); break;
    case 2 : rc = VDatabaseOpenTableRead ( db, &tab, "%s", "SECONDARY_ALIGNMENT" ); break;
    }
    if ( rc == 0 )
    {
        const VCursor *curs;
        rc = VTableCreateCursorRead ( tab, &curs );
        if ( rc ==  0 )
        {
            rc = add_columns( curs, columns );
            if ( rc == 0 )
            {
                rc = VCursorOpen ( curs );
                if ( rc == 0 )
                {
                    const void * ptr;
                    rc = read_string_column( curs, columns[0], data->id, &data->ref_seq_name );
                    if ( rc == 0 )
                    {
                        rc = read_column_direct( curs, columns[1], data->id, &ptr );
                        if ( rc == 0 )
                            data->ref_pos = *( ( INSDC_coord_zero *)ptr );
                    }
                    if ( rc == 0 )
                    {
                        rc = read_column_direct( curs, columns[2], data->id, &ptr );
                        if ( rc == 0 )
                            data->ref_len = *( ( INSDC_coord_zero *)ptr );
                    }
                    if ( rc == 0 )
                        rc = read_string_column( curs, columns[3], data->id, &data->read );
                    if ( rc == 0 )
                        rc = read_string_column( curs, columns[4], data->id, &data->has_mismatch );
                    if ( rc == 0 )
                        rc = read_string_column( curs, columns[5], data->id, &data->has_ref_offset );
                    if ( rc == 0 )
                        rc = read_int32_array( curs, columns[6], data->id, &data->ref_offset, &data->ref_offset_len );
                }
            }
            VCursorRelease ( curs );
        }
        VTableRelease ( tab );
    }
    return rc;
}


static rc_t walk_alignment_iter( AlignmentIterator *it, const ReferenceObj *ref_obj )
{
    const char * refname;
    const char * seq_id;
    rc_t rc = ReferenceObj_Name( ref_obj, &refname );
    if ( rc != 0 )
        return rc;
    rc = ReferenceObj_SeqId( ref_obj, &seq_id );
    if ( rc != 0 )
        return rc;
    do
    {
        char c;
        int32_t pos;
        INSDC_coord_zero seq_pos;
        uint8_t refbase[ 2 ];
        INSDC_coord_len written;

        uint32_t state = AlignmentIteratorState ( it, &seq_pos );
        AlignmentIteratorPosition ( it, &pos );
        rc = ReferenceObj_Read( ref_obj, pos, 1, refbase, &written );
        if ( rc != 0 )
            return rc;
        refbase[ 0 ] = _4na_to_ascii( refbase[ 0 ], false );
        refbase[ 1 ] = 0;

        OUTMSG(( "<%s|%s>[%d] %s [%u] ", refname, seq_id, pos, refbase, seq_pos ));

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
            uint8_t * deleted = malloc( n + 1 );
            if ( deleted != NULL )
            {
                char * s;
                if ( ( state & align_iter_match ) == align_iter_match )
                    OUTMSG(( "MATCH<%c>", c ));
                else
                    OUTMSG(( "MISMATCH<%c>", c ));
            
                rc = ReferenceObj_Read( ref_obj, at, n, deleted, &written );
                if ( rc != 0 )
                    return rc;
                s = dup_2_ascii( deleted, written, false );
                OUTMSG(( "<DEL:%u:at %d = '%s'>\n", n, at, s ));
                free( s );
                free( deleted );
            }
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

    } while( AlignmentIteratorNext ( it ) == 0 && rc == 0 );
    return rc;
}

rc_t walk_1_alignment( Args * args )
{
    rc_t rc = 0;
    trans_opt opt;
    trans_ctx ctx;
    uint32_t table_select = 0;

    alignment_data data;

    memset( &data, 0, sizeof data );

    rc = make_trans_opt( &opt, args );
    if ( rc != 0 )
        LOGERR( klogInt, rc, "make_trans_opt() failed" );

    if ( rc == 0 )
    {
        OUTMSG(( "walking 1 alignment on file >%s<\n", opt.fname ));
        rc = make_trans_ctx( &ctx, &opt, false );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "make_trans_ctx() failed" );
    }

    if ( rc ==  0 )
    {
        data.id = opt.ref_offset[ 0 ];

        if ( opt.ref_name != NULL )
            table_select = ( opt.ref_name[ 0 ] - '0' );

        OUTMSG(( "table_select = %u\n", table_select ));
        rc = discover_ref_seq_name( ctx.db, table_select, &data );

        if ( rc != 0 )
            LOGERR( klogInt, rc, "discover_ref_seq_name() failed" );
        else
        {
            OUTMSG(( "ID >%u< is located on reference '%s' at pos = %u, len = %u\n",
                     data.id, data.ref_seq_name, data.ref_pos, data.ref_len ));
            OUTMSG(( "READ          : >%s<\n", data.read ));
            OUTMSG(( "HAS_MISMATCH  : >%s<\n", data.has_mismatch ));
            OUTMSG(( "HAS_REF_OFFSET: >%s<\n", data.has_ref_offset ));
            print_ref_offset( data.ref_offset, data.ref_offset_len );
        }
    }

    if ( rc == 0 )
    {
        rc = ReferenceList_Find( ctx.ref_list, &ctx.ref_obj,
                                 data.ref_seq_name, string_size( data.ref_seq_name ) );
        if ( rc != 0 )
            LOGERR( klogInt, rc, "ReferenceList_Find() failed" );
        else
            OUTMSG(( "reference found in reflist\n" ));
    }

    if ( rc == 0 )
    {
        AlignmentIterator *it;

        bool * b_hmm = make_bool_array( data.has_mismatch );
        bool * b_hro = make_bool_array( data.has_ref_offset );
        INSDC_4na_bin * bREAD = dup_2_4na( data.read );
        uint32_t read_len = string_size( data.read );

        rc = AlignMgrMakeAlignmentIterator (
                ctx.almgr,      /* the alignment-manager */
                &it,            /* the iterator to be created */
                false,          /* make a copy of the given data */
                data.ref_pos,   /* where the alignment starts, absolute on reference */
                data.ref_len,   /* how long the alignment is, against the reference */
                bREAD,          /* 4na of READ */
                read_len,       /* read-len */
                b_hmm,          /* has-mismatch (array of bool) */
                b_hro,          /* has-ref-offset (array of bool) */
                data.ref_offset,        /* ref-offset (array of int32 ) */
                data.ref_offset_len,    /* how many ref-offset-values in array */
                data.ref_pos,   /* where the loading, window starts, for this test == ref_pos */
                data.ref_len    /* how long the loading window is, for this test == ref_len */
                );
        if ( rc == 0 )
        {
            rc = walk_alignment_iter( it, ctx.ref_obj );
            AlignmentIteratorRelease ( it );
        }
        else
            LOGERR( klogInt, rc, "AlignMgrMakeAlignmentIterator() failed" );

        free( b_hmm );
        free( b_hro );
        free( bREAD );
    }

    free_alignment_data( &data );
    free_trans_ctx( &ctx );
    return rc;
}