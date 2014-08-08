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

#include <kdb/meta.h>
#include <kdb/namelist.h>
#include <align/reference.h>
#include <sysalloc.h>

#include "sam-dump-opts.h"
#include "inputfiles.h"


static rc_t print_headers_from_metadata( const VDatabase * db, bool * recalc, const char * path )
{
    const KMetadata * meta;
    rc_t rc = VDatabaseOpenMetadataRead( db, &meta );
    if ( rc != 0 )
        (void)PLOGERR( klogErr, ( klogErr, rc, "cannot open metadata from '$(t)'", "t=%s", path ) );
    else
    {
        const KMDataNode * node;
        rc = KMetadataOpenNodeRead( meta, &node, "BAM_HEADER" );
        if ( rc != 0 )
        {
            if ( GetRCState( rc ) == rcNotFound )
            {
                *recalc = true;
                rc = 0;
            }
            else
                (void)PLOGERR( klogErr, ( klogErr, rc, "cannot open meta-node 'BAM_HEADER' from '$(t)'", "t=%s", path ) );
        }
        else
        {
            size_t offset = 0, num_read, remaining = ~0;
            char buffer[ 40960 ];
            while ( rc == 0 && remaining > 0 )
            {
                rc = KMDataNodeRead( node, offset, buffer, sizeof( buffer ),
                                     &num_read, &remaining );
                if ( rc == 0 )
                {
                    rc = KOutMsg( "%.*s", ( uint32_t )num_read, buffer );
                    offset += num_read;
                }
            }
            if ( rc == 0 && buffer[ num_read - 1 ] != '\n' )
            {
                rc = KOutMsg( "\n" );
            }
            KMDataNodeRelease( node );
        }
        KMetadataRelease( meta );
    }
    return rc;
}


typedef struct seq_id_node
{
    BSTNode node;
    char * seq_id;
    char * name;
    INSDC_coord_len seq_len;
} seq_id_node;


static int cmp_pchar( const char * a, const char * b )
{
    int res = 0;
    if ( ( a != NULL )&&( b != NULL ) )
    {
        size_t len_a = string_size( a );
        size_t len_b = string_size( b );
        res = string_cmp ( a, len_a, b, len_b, ( len_a < len_b ) ? len_b : len_a );
    }
    return res;
}


static seq_id_node * make_seq_id_node( const char * seq_id, const char * name, INSDC_coord_len seq_len )
{
    seq_id_node *res = calloc( sizeof *res, 1 );
    if ( res != NULL )
    {
        res->name = string_dup( name, string_size( name ) );
        res->seq_id = string_dup( seq_id, string_size( seq_id ) );
        res->seq_len = seq_len;
    }
    return res;
}


static void free_seq_id_node( seq_id_node * node )
{
    if ( node != NULL )
    {
        if ( node->seq_id != NULL )
            free( node->seq_id );
        if ( node->name != NULL )
            free( node->name );
        free( node );
    }
}


static void CC free_seq_id_tree_callback( BSTNode *n, void * data )
{
    free_seq_id_node( ( seq_id_node * )n );
}


static void free_seq_id_tree( BSTree * tree )
{
    BSTreeWhack( tree, free_seq_id_tree_callback, NULL );
}


static int CC seq_id_node_vs_pchar_wrapper( const void *item, const BSTNode *n )
{
    const seq_id_node * node = ( const seq_id_node * )n;
    return cmp_pchar( (const char *)item, node->seq_id );
}


static seq_id_node * find_seq_id_node( BSTree * tree, const char * key )
{
    return ( seq_id_node * ) BSTreeFind( tree, key, seq_id_node_vs_pchar_wrapper );
}


static int CC node_vs_node_wrapper( const BSTNode *item, const BSTNode *n )
{
   const seq_id_node * a = ( const seq_id_node * )item;
   const seq_id_node * b = ( const seq_id_node * )n;
   return cmp_pchar( a->seq_id, b->seq_id );
}


static rc_t add_seq_id_node( BSTree * tree, const char * seq_id, const char * name, INSDC_coord_len len )
{
    rc_t rc = 0;
    seq_id_node * node = make_seq_id_node( seq_id, name, len );
    if ( node != NULL )
    {
        rc = BSTreeInsert( tree, (BSTNode *)node, node_vs_node_wrapper );
        if ( rc != 0 )
            free_seq_id_node( node );
    }
    else
        rc = RC( rcApp, rcNoTarg, rcConstructing, rcMemory, rcExhausted );
    return rc;
}


static rc_t print_seq_id_tree( BSTree * tree, input_files * ifs )
{
    rc_t rc = 0;
    uint32_t i;
    for ( i = 0; i < ifs->database_count && rc == 0; ++i )
    {
        input_database * id = VectorGet( &ifs->dbs, i );
        if ( id != NULL )
        {
            uint32_t rcount;
            rc = ReferenceList_Count( id->reflist, &rcount );
            if ( rc == 0 && rcount > 0 )
            {
                uint32_t r_idx;
                for ( r_idx = 0; r_idx < rcount && rc == 0; ++r_idx )
                {
                    const ReferenceObj * ref_obj;
                    rc = ReferenceList_Get( id->reflist, &ref_obj, r_idx );
                    if ( rc == 0 && ref_obj != NULL )
                    {
                        const char * seqid;
                        rc = ReferenceObj_SeqId( ref_obj, &seqid );
                        if ( rc == 0 )
                        {
                            const char * name;
                            rc = ReferenceObj_Name( ref_obj, &name );
                            if ( rc == 0 )
                            {
                                INSDC_coord_len seq_len;
                                rc = ReferenceObj_SeqLength( ref_obj, &seq_len );
                                if ( rc == 0 )
                                {
                                    seq_id_node * node = find_seq_id_node( tree, seqid );
                                    if ( node != NULL )
                                    {
                                        if ( node->seq_len != seq_len )
                                            rc = RC( rcApp, rcNoTarg, rcConstructing, rcData, rcInvalid );
                                    }
                                    else
                                        rc = add_seq_id_node( tree, seqid, name, seq_len );
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return rc;
}


typedef struct hdr_print_ctx
{
    rc_t rc;
    bool use_seq_id;
} hdr_print_ctx;


static void CC print_header_callback( BSTNode *n, void *data )
{
    seq_id_node * node = ( seq_id_node * )n;
    hdr_print_ctx * hctx = ( hdr_print_ctx * )data;
    if ( hctx->rc == 0 )
    {
        if ( hctx->use_seq_id )
            hctx->rc = KOutMsg( "@SQ\tSN:%s\tLN:%u\n", node->seq_id, node->seq_len );
        else
        {
            if ( cmp_pchar( node->seq_id, node->name ) == 0 )
                hctx->rc = KOutMsg( "@SQ\tSN:%s\tLN:%u\n", node->name, node->seq_len );
            else
                hctx->rc = KOutMsg( "@SQ\tSN:%s\tAS:%s\tLN:%u\n", node->name, node->seq_id, node->seq_len );
        }
    }
}


static rc_t extract_spotgroups( VNamelist * spotgroups, input_files * ifs )
{
    rc_t rc = 0;
    uint32_t i;
    for ( i = 0; i < ifs->database_count && rc == 0; ++i )
    {
        input_database * id = VectorGet( &ifs->dbs, i );
        if ( id != NULL )
        {
            const VTable * tab;
            rc = VDatabaseOpenTableRead( id->db, &tab, "SEQUENCE" );
            if ( rc != 0 )
                (void)PLOGERR( klogErr, ( klogErr, rc, "cannot open table SEQUENCE in '$(t)'", "t=%s", id->path ) );
            else
            {
                const KMetadata * meta;
                rc = VTableOpenMetadataRead( tab, &meta );
                if ( rc != 0 )
                    (void)PLOGERR( klogErr, ( klogErr, rc, "cannot open metadata from '$(t)'", "t=%s", id->path ) );
                else
                {
                    const KMDataNode * node;
                    rc = KMetadataOpenNodeRead( meta, &node, "STATS/SPOT_GROUP" );
                    if ( rc != 0 )
                       (void)PLOGERR( klogErr, ( klogErr, rc, "cannot open meta-node 'STATS/SPOT_GROUP' from '$(t)'", "t=%s", id->path ) );
                    else
                    {
                        KNamelist * node_childs;
                        rc = KMDataNodeListChildren( node, &node_childs );
                        if ( rc != 0 )
                            (void)PLOGERR( klogErr, ( klogErr, rc, "cannot list children of SPOT_GROUP-node in '$(t)'", "t=%s", id->path ) );
                        else
                        {
                            uint32_t n_count;
                            rc = KNamelistCount( node_childs, &n_count );
                            if ( rc == 0 && n_count > 0 )
                            {
                                uint32_t n_idx;
                                for ( n_idx = 0; n_idx < n_count && rc == 0; ++n_idx )
                                {
                                    const char * spotgroup;
                                    rc = KNamelistGet( node_childs, n_idx, &spotgroup );
                                    if ( rc == 0 && spotgroup != NULL )
                                    {
                                        uint32_t found;
                                        rc_t rc1 = VNamelistIndexOf( spotgroups, spotgroup, &found );
                                        if ( GetRCState( rc1 ) == rcNotFound )
                                            rc = VNamelistAppend( spotgroups, spotgroup );
                                    }
                                }
                            }
                            KNamelistRelease( node_childs );
                        }
                        KMDataNodeRelease( node );
                    }
                    KMetadataRelease( meta );
                }
                VTableRelease( tab );
            }
        }
    }
    return rc;
}


static rc_t print_spotgroups( VNamelist * spotgroups )
{
    uint32_t count;
    rc_t rc = VNameListCount( spotgroups, &count );
    if ( rc == 0 && count > 0 )
    {
        uint32_t i;
        for ( i = 0; i < count && rc == 0; ++i )
        {
            const char * s;
            rc = VNameListGet( spotgroups, i, &s );
            if ( rc == 0 && s != NULL )
            {
                if ( cmp_pchar( s, "default" ) != 0 )
                    rc = KOutMsg( "@RG\tID:%s\n", s );
            }
        }
    }
    return rc;
}


static rc_t print_headers_by_recalculating( samdump_opts * opts, input_files * ifs )
{
    BSTree tree;
    rc_t rc = KOutMsg( "@HD\tVN:1.3\n" );
    if ( rc == 0 )
    {
        /* collect sequenc-id's and names and their lengths, unique by sequence-id */
        BSTreeInit( &tree );
        rc = print_seq_id_tree( &tree, ifs );
        if ( rc == 0 )
        {
            hdr_print_ctx hctx;
            hctx.rc = 0;
            hctx.use_seq_id = opts->use_seqid_as_refname;
            /* ptrint it */
            BSTreeForEach( &tree, false, print_header_callback, &hctx );
            rc = hctx.rc;
        }
        free_seq_id_tree( &tree );

        /* collect spot-groups ( unique ) */
        if ( rc == 0 )
        {
            VNamelist * spotgroups;
            rc = VNamelistMake( &spotgroups, 10 );
            if ( rc == 0 )
            {
                rc = extract_spotgroups( spotgroups, ifs );
                if ( rc == 0 )
                    rc = print_spotgroups( spotgroups );
                VNamelistRelease( spotgroups );
            }
        }
    }
    return rc;
}


rc_t print_headers( samdump_opts * opts, input_files * ifs )
{
    rc_t rc = 0;
    if ( ifs->database_count > 1 )
        rc = print_headers_by_recalculating( opts, ifs );
    else
    {
        bool recalc = false;
        switch( opts->header_mode )
        {
        case hm_dump    :   {
                                input_database * id = VectorGet( &ifs->dbs, 0 );
                                rc = print_headers_from_metadata( id->db, &recalc, id->path );
                                if ( rc == 0 && recalc )
                                    rc = print_headers_by_recalculating( opts, ifs );
                            }
                            break;

        case hm_recalc  :   rc = print_headers_by_recalculating( opts, ifs );
                            break;

        case hm_none    :   break; /* to not let the compiler complain about not handled enum */
        }
    }

    /* attach header comments from the commandline */
    if ( rc == 0 && opts->hdr_comments != NULL )
    {
        uint32_t count;
        rc = VNameListCount( opts->hdr_comments, &count );
        if ( rc == 0 && count > 0 )
        {
            uint32_t i;
            for ( i = 0; i < count && rc == 0; ++i )
            {
                const char * s;
                rc = VNameListGet( opts->hdr_comments, i, &s );
                if ( rc == 0 && s != NULL )
                    rc = KOutMsg( "@CO\t%s\n", s );
            }
        }
    }
    return rc;
}
