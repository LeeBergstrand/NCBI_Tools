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

#include "kdf5-test.vers.h"

#include <klib/out.h>
#include <klib/namelist.h>
#include <klib/text.h>
#include <klib/rc.h>
#include <kapp/main.h>
#include <hdf5/kdf5.h>
#include <kfs/arrayfile.h>

#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <common/test_assert.h>

const char UsageDefaultName[] = "kdf5-test";
#define def_name UsageDefaultName

rc_t CC UsageSummary ( const char * progname )
 {
     OUTMSG ( ("\n"
         "Usage:\n"
         "  %s <kdf5-file>\n"
         "\n", progname) );
    return 0;
}

rc_t CC Usage ( const Args * args )
{
    const char * progname;
    const char * fullpath;
    rc_t rc;

    if ( args == NULL )
        rc = RC (rcApp, rcArgv, rcAccessing, rcSelf, rcNull);
    else
        rc = ArgsProgram (args, &fullpath, &progname);
    if ( rc )
        progname = fullpath = def_name;

    UsageSummary ( progname );

    HelpOptionsStandard ();

    HelpVersion ( fullpath, KAppVersion() );

    return rc;
}


/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return KDF5_TEST_VERS;
}


static
void kdf5_show_obj_type( const uint32_t obj_type )
{
    uint32_t x = obj_type;
    if ( ( x & kptAlias ) > 0 )
    {
        OUTMSG(( "alias of " ));
        x &= ~kptAlias;
    }

    switch( x )
    {
    case kptNotFound   : OUTMSG(( "kptNotFound" )); break;
    case kptBadPath    : OUTMSG(( "kptBadPath" )); break;
    case kptFile       : OUTMSG(( "kptFile" )); break;
    case kptDir        : OUTMSG(( "kptDir" )); break;
    case kptCharDev    : OUTMSG(( "kptCharDev" )); break;
    case kptBlockDev   : OUTMSG(( "kptBlockDev" )); break;
    case kptFIFO       : OUTMSG(( "kptFIFO" )); break;
    case kptZombieFile : OUTMSG(( "kptZombieFile" )); break;
    case kptDataset    : OUTMSG(( "kptDataset" )); break;
    case kptDatatype   : OUTMSG(( "kptDatatype" )); break;
    default            : OUTMSG(( "unknown kpt*" )); break;
    }
}


static
rc_t kdf5_test1( KDirectory * hdf5_dir, const char * path )
{
    KNamelist * groups;
    rc_t rc;

    OUTMSG (( "---TEST: KDirectoryList(%s)---------------\n", path ));
    rc = KDirectoryList ( hdf5_dir, &groups, NULL, NULL, path );
    if ( rc != 0 )
        OUTMSG (( "KDirectoryList() = %R\n", rc ));
    if ( rc == 0 )
    {
        uint32_t idx, count;
        rc = KNamelistCount ( groups, &count );
        for ( idx = 0; idx < count && rc == 0; ++idx )
        {
            const char * name;
            rc = KNamelistGet ( groups, idx, &name );
            if ( rc == 0 )
            {
                uint32_t obj_type;
                if ( path != NULL )
                {
                    char buffer[ 1024 ];
                    sprintf( buffer, "%s/%s", path, name );
                    obj_type = KDirectoryPathType ( hdf5_dir, buffer );
                }
                else
                    obj_type = KDirectoryPathType ( hdf5_dir, name );

                OUTMSG (( "list #%d = '%s' (", idx, name ));
                kdf5_show_obj_type( obj_type );
                OUTMSG (( ")\n" ));
            }
        }
        KNamelistRelease( groups );
    }

    return rc;
}


static
rc_t CC visit_test2_cb( const KDirectory *dir, uint32_t type, const char *name, void *data )
{
    int * idx = data;

    OUTMSG (( "visit #%d= '%s' ( ", *idx, name ));
    kdf5_show_obj_type( type );

    if ( type == kptDataset )
    {
        /* try to open it... */
        struct KFile const *f;
        rc_t rc = KDirectoryOpenFileRead ( dir, &f, name );
        if ( rc == 0 )
        {
            uint64_t size;
            rc = KFileSize ( f, &size );
            if ( rc == 0 )
                OUTMSG (( ", size = %lu", size ));
            else
                OUTMSG (( ", size = ? %R", rc ));
            KFileRelease( f );
        }
    }
    OUTMSG (( " )\n" ));

    ( *idx )++;
    return 0;
}


static
rc_t kdf5_test2( KDirectory * hdf5_dir, bool recurse )
{
    int idx = 0;
    rc_t rc;

    OUTMSG (( "--------------------------------------\n" ));
    rc = KDirectoryVisit ( hdf5_dir, recurse, visit_test2_cb, &idx, NULL );
    if ( rc != 0 ) OUTMSG (( "KDirectoryVisit() = %R\n", rc ));

    return rc;
}


static
void show_extends( const uint8_t dim, uint64_t * extents )
{
    uint8_t i;
    OUTMSG (( "%lu", extents[0] ));
    for ( i = 1; i < dim; ++i )
        OUTMSG (( " x %lu", extents[i] ));
}


static
int prepare_buffers( const uint8_t dim, uint64_t ** b1, uint64_t ** b2, uint64_t ** b3 )
{
    *b1 = malloc( dim * ( sizeof ( uint64_t ) ) );
    if ( b1 == NULL ) return 0;
    *b2 = malloc( dim * ( sizeof ( uint64_t ) ) );
    if ( b2 != NULL )
    {
        *b3 = malloc( dim * ( sizeof ( uint64_t ) ) );
        if ( *b3 != NULL )
            return 1;
        free( *b2 );
    }
    free( *b1 );
    return 0;
}


static
uint64_t calc_total_element_count( const uint8_t dim, uint64_t *v )
{
    uint8_t i;
    uint64_t res = v[ 0 ];
    for ( i = 1; i < dim; ++i )
        res *= v[ i ];
    return res;
}


static
void show_buffer( char * buffer, uint64_t bytes )
{
    uint64_t i;

    if ( bytes > 16 ) bytes = 16;
    OUTMSG(( "-> element: %.02X", buffer[ 0 ] & 0xFF ));
    for ( i = 1; i < bytes; ++i )
        OUTMSG(( ":%.02X", buffer[ i ] & 0xFF ));
    OUTMSG(( "\n" ));
}


static
void read_something( KArrayFile *af, const uint8_t dim, 
                     uint64_t * extents, uint64_t elem_bytes )
{
    uint64_t * pos;
    uint64_t * cnt;
    uint64_t * n_read;

    if ( prepare_buffers( dim, &pos, &cnt, &n_read ) > 0 )
    {
        uint8_t i;
        char * buffer;

        for ( i = 0; i < dim; ++i )
        {
            pos[ i ] = 0;
            if ( extents[ i ] > 1 )
            cnt[ i ] = 1;

        }

        /* we read exactly 1 element! */
        buffer = malloc( elem_bytes );
        if ( buffer != NULL )
        {
            rc_t rc = KArrayFileRead ( af, dim, pos, buffer, cnt, n_read );
            if ( rc == 0 )
            {
                uint64_t total_read = calc_total_element_count( dim, n_read );
                OUTMSG(( "%u elements were read\n", total_read ));
                show_buffer( buffer, elem_bytes );
            }
            else
                OUTMSG(( "KArrayFileRead() = %R", rc ));

            free( buffer );
        }
        free( n_read );
        free( cnt );
        free( pos );
    }
}


static
void visit_afile( KArrayFile *af )
{
    uint8_t dim = 0;
    uint64_t * extents;

    rc_t rc = KArrayFileDimensionality ( af, &dim );
    if ( rc != 0 ) OUTMSG (( "KArrayFileDimensionality() = %R\n", rc ));
    OUTMSG (( "-> dim : %d\n", dim ));
    extents = malloc( dim * ( sizeof ( uint64_t ) ) );
    if ( extents != NULL )
    {
        uint64_t elem_bits, elem_bytes;

        rc = KArrayFileDimExtents ( af, dim, extents );
        if ( rc != 0 ) OUTMSG (( "KArrayFileDimExtents() = %R\n", rc ));
        OUTMSG (( "-> extents : " ));
        show_extends( dim, extents );
        OUTMSG(( "\n" ));

        rc = KArrayFileElementSize ( af, &elem_bits );
        if ( rc != 0 ) OUTMSG (( "KArrayFileElementSize() = %R\n", rc ));
        OUTMSG (( "-> element_bits : %u\n", elem_bits ));
        elem_bytes = ( elem_bits >> 3 );
        if ( ( elem_bits - ( elem_bytes << 3 ) ) > 0 )
            elem_bytes++;
        OUTMSG (( "-> element_bytes : %u\n", elem_bytes ));
        read_something( af, dim, extents, elem_bytes );

        free ( extents );
    }
}


static
rc_t CC visit_test3_cb( const KDirectory *dir, uint32_t type, const char *name, void *data )
{
    struct KFile const *f;
    struct KArrayFile *af;
    rc_t rc;
    int * idx = data;

    if ( type != kptDataset ) return 0;
    rc = KDirectoryOpenFileRead ( dir, &f, name );
    if ( rc != 0 ) return 0;

    rc = MakeHDF5ArrayFile ( f, &af );
    if ( rc == 0 )
    {
        OUTMSG (( "visit arrayfile #%d '%s':\n", *idx, name ));
        visit_afile( af );
        KArrayFileRelease( af );
    }
    else
        OUTMSG (( "visit arrayfile #%d '%s' -> %R\n", *idx, name, rc ));

    KFileRelease( f );
    ( *idx )++;
    OUTMSG(( "\n" ));
    return 0;
}


static
rc_t kdf5_test3( KDirectory * hdf5_dir, bool recurse )
{
    int idx = 0;
    rc_t rc;

    OUTMSG (( "--------------------------------------\n" ));
    rc = KDirectoryVisit ( hdf5_dir, recurse, visit_test3_cb, &idx, NULL );
    if ( rc != 0 ) OUTMSG (( "KDirectoryVisit() = %R\n", rc ));

    return rc;
}


static
rc_t kdf5_main( const char * filename, const char * datasetname )
{
    rc_t rc;
    KDirectory * native_dir;
    KDirectory * hdf5_dir;

    rc = KDirectoryNativeDir ( & native_dir );
    OUTMSG (( "KDirectoryNativeDir() = %R\n", rc ));
    if ( rc != 0 ) return rc;

    rc = MakeHDF5RootDir ( native_dir, &hdf5_dir, false, filename );
    OUTMSG (( "MakeHDF5RootDir() = %R\n", rc ));
    if ( rc == 0 )
    {
#if 0
        /* calls KDirectoryList() and KDirectoryPathType() */
        rc = kdf5_test1( hdf5_dir, NULL );
        OUTMSG (( "\n" ));

        rc = kdf5_test1( hdf5_dir, "arrays" );
        OUTMSG (( "\n" ));

        /* calls KDirectoryVisit( no recursion ) */
        rc = kdf5_test2( hdf5_dir, false );
        OUTMSG (( "\n" ));

        /* calls KDirectoryVisit( with recursion ) */
        rc = kdf5_test2( hdf5_dir, true );
        OUTMSG (( "\n" ));
#endif
        /* calls KDirectoryVisit( with recursion ) */
        rc = kdf5_test3( hdf5_dir, true );
        OUTMSG (( "\n" ));

        KDirectoryRelease( hdf5_dir );
    }
    KDirectoryRelease( native_dir );
    return rc;
}


static
rc_t file_copy( const KFile *src, KFile *dst, size_t bufsize, uint64_t * copied )
{
    rc_t rc = 0;
    uint64_t pos = 0;
    size_t num_read = 1;
    char * buffer;

    *copied = 0;
    buffer = malloc( bufsize );
    if ( buffer == NULL )
        return RC( rcExe, rcFile, rcPacking, rcMemory, rcExhausted );

    while ( rc == 0 && num_read > 0 )
    {
        rc = KFileRead ( src, pos, buffer, bufsize, &num_read );
        if ( rc != 0 )
            OUTMSG(( "KFileRead() failed %R\n", rc ));
        if ( rc == 0 && num_read > 0 )
        {
            size_t num_writ;
            rc = KFileWrite ( dst, pos, buffer, num_read, &num_writ );
            if ( rc != 0 )
                OUTMSG(( "KFilewrite() failed %R\n", rc ));
            pos += num_read;
        }
    }
    free( buffer );
    *copied = pos;
    return rc;
}


static
char * make_dst_filename( const char * src )
{
    size_t size;
    char * res = string_dup_measure ( src, &size );
    if ( res != NULL )
    {
        size_t i;
        for ( i = 0; i < size; ++i )
        {
            if ( res[ i ] == ' ' || res[ i ] == '/' )
                res[ i ] = '_';
        }
    }
    return res;
}


static
rc_t read_dataset_into_file( const char * filename, const char * datasetname )
{
    rc_t rc;
    KDirectory * native_dir;
    KDirectory * hdf5_dir;

    rc = KDirectoryNativeDir ( & native_dir );
    OUTMSG (( "KDirectoryNativeDir() = %R\n", rc ));
    if ( rc != 0 ) return rc;

    rc = MakeHDF5RootDir ( native_dir, &hdf5_dir, false, filename );
    OUTMSG (( "MakeHDF5RootDir() = %R\n", rc ));
    if ( rc == 0 )
    {
        struct KFile const *src_file;
        rc = KDirectoryOpenFileRead ( hdf5_dir, &src_file, datasetname );
        OUTMSG (( "KDirectoryOpenFileRead( hdf5_dir ) = %R\n", rc ));
        if ( rc == 0 )
        {
            char * fn = make_dst_filename( datasetname );
            if ( fn != NULL )
            {
                KFile * dst_file;
                rc = KDirectoryCreateFile ( native_dir, &dst_file, false, 0664, kcmInit, fn );
                OUTMSG (( "KDirectoryCreateFile( native_dir ) = %R\n", rc ));
                if ( rc == 0 )
                {
                    uint64_t copied;
                    rc = file_copy( src_file, dst_file, 4096, &copied );
                    OUTMSG (( "%lu bytes copied\n", copied ));
                    KFileRelease( src_file );
                }
                free( fn );
            }
            KFileRelease( src_file );
        }
        KDirectoryRelease( hdf5_dir );
    }
    KDirectoryRelease( native_dir );
    return rc;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    if ( argc < 3 )
    {
        OUTMSG (( "filename and dataset are missing!\n" ));
        return -1;
    }
#if 0
    return kdf5_main( argv[1], argv[2] );
#endif
    return read_dataset_into_file( argv[1], argv[2] );
}

