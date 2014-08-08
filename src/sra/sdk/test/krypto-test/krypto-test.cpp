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

#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>

#include <ktst/unit_test.hpp>
#include <krypto/key.h>
#include <krypto/encfile.h>
#include <krypto/encfile-priv.h>
#include <krypto/reencfile.h>
#include <kfs/impl.h>
#include <kfs/buffile.h>
#include <klib/out.h>
#include <klib/vector.h>
#include <kapp/args.h>

#include <common/test_assert.h>

using namespace std;

TEST_SUITE( KryptoTestSuite );

/* ------------------------------------------------------------------------------------------- */

string create_random_pwd( size_t len )
{
    string res;
    while ( res.size() < len )
    {
        unsigned char c = rand();
        if ( ( c != 0 )&&( c != '\n' )&& ( c != '\r' ) )
            res += c;
    }
    return res;
}


size_t encrypted_size( size_t plain_size )
{
    return ( ( ( ( plain_size + 32767 ) / 32768 ) * ( 32768 + 64 ) ) + 32 );
}


/* ------------------------------------------------------------------------------------------- */

void show_diff( const char * buf1, const char *buf2, size_t len )
{
    cout << endl;
    unsigned int i, n_diff = 0;
    for ( i = 0; i < len && n_diff < 100; ++i )
    {
        if ( buf1[ i ] != buf2[ i ] )
        {
            cout << dec << i;
            cout << hex << ": 0x" << (int)( buf1[ i ] & 0xFF ) << " != 0x" << (int)( buf2[ i ] & 0xFF ) << endl;
            n_diff++;
        }
    }
    cout << endl;
}


class CryptoFixture
{
public:
    KDirectory * wd;
    KKey key;
    string PlainName;
    string EncName;
    string Password;
    unsigned int PlainLength;
    int seed;
    unsigned int PwdLength;
    unsigned int EncBufSize;
    KKeyType k_type;
    char * PatternBuffer;
    unsigned int PatternLength;
    bool verify;
    size_t pre_decrypt_buffer;
    size_t post_decrypt_buffer;
    bool enc_update_mode;

    // the constructor
    CryptoFixture() : wd( NULL ), PlainName( "plain.bin" ), EncName( "enc.bin" ), Password( "" ),
                      PlainLength( 50000 ), seed( 0 ), PwdLength( 100 ), EncBufSize( 1024 ), 
                      k_type( kkeyAES256 ), PatternBuffer( NULL ), PatternLength( 0 ), verify( false ),
                      pre_decrypt_buffer( 0 ), post_decrypt_buffer( 0 ), enc_update_mode( true )

    {
        rc_t rc = KDirectoryNativeDir ( & wd );
        if ( rc != 0 )
            FAIL( "KDirectoryNativeDir failed" );
    }

    // the destructor
    ~CryptoFixture()
    {
        if ( PatternBuffer != NULL )
            free( PatternBuffer );
        rc_t rc = KDirectoryRelease ( wd );
        if ( rc != 0 )
            FAIL( "KDirectoryRelease failed" );
    }

    // setters for different parameters
    void set_PlainName( string &value ) { PlainName = value; }
    void set_PlainLength( unsigned int value ) { PlainLength = value; }
    void set_Seed( int value ) { seed = value; }
    void set_PwdLength( int value ) { PwdLength = value; }
    void set_EncName( string &value ) { EncName = value; }
    void set_KeyType( KKeyType value ) { k_type = value; }
    void set_EncBufSize( unsigned int value ) { EncBufSize = value; }
    void set_Verify( bool value ) { verify = value; }
    void set_PreDecBuffer( size_t value ) { pre_decrypt_buffer = value; }
    void set_PostDecBuffer( size_t value ) { post_decrypt_buffer = value; }
    void set_EncUpdateMode( bool value ) { enc_update_mode = value; }

    void set_Pattern( const char * pattern, unsigned int len )
    {
        if ( len > 0 && pattern != NULL )
        {
            PatternBuffer = ( char * ) malloc( len );
            if ( PatternBuffer != NULL )
            {
                memcpy( PatternBuffer, pattern, len );
                PatternLength = len;
            }
            else
                PatternLength = 0;
        }
        else
        {
            if ( PatternBuffer != NULL )
                free( PatternBuffer );
            PatternBuffer = NULL;
            PatternLength = 0;
        }
    }

    // initialize
    bool initialize( void )
    {
        srand( seed );
        bool res;
        if ( PatternBuffer != NULL && PatternLength > 0 )
            res = create_pattern_plainfile();
        else
            res = create_random_plainfile();
        if ( !res )
            FAIL( "plainfile not created!" );
        else
        {
            res = create_key();
            if ( !res )
                FAIL( "key not created!" );
            else
            {
                res = encrypt();
                if ( !res )
                    FAIL( "encrypt failed!" );
            }
        }
        return res;
    }


    bool create_random_plainfile( void )
    {
        ofstream outputFile( PlainName.c_str() );
        if ( outputFile == NULL )
            FAIL( "cannot create plaintext-file" );
        else
        {
            unsigned int i;
            for ( i = 0; i < PlainLength; ++i )
            {
                unsigned char c = rand();
                outputFile << c;
            }
            outputFile.close();
        }
        return true;
    } 


    bool create_pattern_plainfile( void )
    {
        bool res = false;
        if ( PatternBuffer == NULL || PatternLength == 0 )
            FAIL( "cannot create pattern-plainfile, no pattern defined" );
        else
        {
            ofstream outputFile( PlainName.c_str() );
            if ( outputFile == NULL )
                FAIL( "cannot create plaintext-file" );
            else
            {
                unsigned int pos = 0;
                while( pos < PlainLength && outputFile.good() )
                {
                    unsigned int to_write = PatternLength;
                    if ( pos + to_write > PlainLength )
                        to_write = PlainLength - pos;
                    outputFile.write( PatternBuffer, to_write );
                    pos += to_write;
                }
                res = ( pos == PlainLength );
                outputFile.close();
            }
        }
        return res;
    }


    bool create_key( void )
    {
        Password.clear();
        while ( Password.size() < PwdLength )
        {
            unsigned char c = rand();
            if ( ( c != 0 )&&( c != '\n' )&& ( c != '\r' ) )
                Password += c;
        }
        rc_t rc = KKeyInitUpdate( &key, k_type, Password.c_str(), Password.length() );
        if ( rc != 0 )
            FAIL( "KKeyInitUpdate() failed" );
        return true;
    }


    bool encrypt( void )
    {
        bool res = false;
        KFile * wrapped;
        rc_t rc = KDirectoryCreateFile( wd, &wrapped, true, 0660, kcmInit, EncName.c_str() );
        if ( rc != 0 )
            FAIL( "KDirectoryCreateFile() failed" );
        else
        {
            KFile * encrypted = NULL;
            if ( enc_update_mode )
            {
                rc = KEncFileMakeUpdate( &encrypted, wrapped, &key );
                if ( rc != 0 )
                    FAIL( "KEncFileMakeUpdate() failed" );
            }
            else
            {
                rc = KEncFileMakeWrite( &encrypted, wrapped, &key );
                if ( rc != 0 )
                    FAIL( "KEncFileMakeWrite() failed" );
            }
            if ( rc == 0 )
            {
                ifstream inputFile( PlainName.c_str(), ifstream::in | ifstream::binary );
                if ( inputFile == NULL )
                    FAIL( "cannot open plainfile for read" );
                else
                {
                    char * buffer = new char[ EncBufSize ];
                    if ( buffer != NULL )
                    {
                        uint64_t pos = 0;
                        do
                        {
                            inputFile.read( buffer, EncBufSize );
                            streamsize read = inputFile.gcount();
                            if ( read > 0 )
                            {
                                size_t num_writ;
                                rc = KFileWriteAll( encrypted, pos, buffer, (size_t)read, &num_writ );
                                if ( rc != 0 )
                                    FAIL( "KFileWriteAll() failed" );
                                pos += num_writ;
                                // res += num_writ; // ???
                            }
                        } while( !inputFile.eof() && rc == 0 );
                        res = true;
                        delete[] buffer;
                    }
                    inputFile.close();
                }
                KFileRelease( encrypted );
            }
            KFileRelease( wrapped );
        }
        if ( res )
        {
            unsigned int enc_file_size = size_of_file( EncName.c_str() );
            KOutMsg( "encrypted file created! ( %,u bytes )\n", enc_file_size );

            // did we create the right size?
            if ( enc_file_size != encrypted_size( PlainLength ) )
                FAIL( "size of created encrypted file is not what we expect!" );
        }
        return res;
    }


    bool compare_decrypted_vs_plain( ifstream &plain, KFile * decrypted, size_t pos, size_t len )
    {
        bool res = false;
        char * plainbuf = new char[ len ];
        if ( plainbuf == NULL )
            FAIL( "cannot allocate buffer to compare plain vs decrypted" );
        else
        {
            plain.seekg( pos, ios::beg );
            plain.read( plainbuf, len );
            char * decryptbuf = new char[ len ];
            if ( decryptbuf == NULL )
                FAIL( "cannot allocate buffer to compare plain vs decrypted" );
            else
            {
                size_t num_read;
                rc_t rc = KFileReadAll( decrypted, pos, decryptbuf, len, &num_read );
                if ( rc != 0 )
                    FAIL( "cannot read decrypted data" );
                else
                {
                    res = ( memcmp( plainbuf, decryptbuf, num_read ) == 0 );
                    if ( !res )
                        show_diff( plainbuf, decryptbuf, num_read );
                }
                delete[] decryptbuf;
            }
            delete[] plainbuf;
        }
        return res;
    }

    
    bool random_access_decryption( unsigned int count, bool read_only )
    {
        KFile * decrypted;
        bool res;
        if ( read_only ) 
            res = make_decrypted_read_only_file( (const KFile **)&decrypted );
        else
            res = make_decrypted_read_write_file( &decrypted );
        if ( res )
        {
            ifstream plain( PlainName.c_str(), ifstream::in | ifstream::binary );
            if ( plain == NULL )
                FAIL( "cannot open plaintext-file for comparison" );
            else
            {
                unsigned int i;
                bool fine = true;
                for ( i = 0; i < count && fine; ++i )
                {
                    size_t pos = rand() % PlainLength;
                    size_t len = rand() % PlainLength;
                    if ( pos + len >= PlainLength )
                        len = ( PlainLength - pos );
                    fine = compare_decrypted_vs_plain( plain, decrypted, pos, len );
                    if ( fine )
                        KOutMsg( "[%,u].[%,u] -> correct\n", pos, len );
                    else
                        KOutMsg( "[%,u].[%,u] -> failed\n", pos, len );
                    plain.clear();
                }
                res = fine;
                plain.close();
            }
            KFileRelease( decrypted );
        }
        return res;
    }


    bool continous_decryption( unsigned int start, int block_inc, int count, unsigned int block_size )
    {
        KFile * decrypted;
        bool res = make_decrypted_read_only_file( (const KFile **)&decrypted );
        if ( res )
        {
            ifstream plain( PlainName.c_str(), ifstream::in | ifstream::binary );
            if ( plain == NULL )
                FAIL( "cannot open plaintext-file for comparison" );
            else
            {
                size_t pos = start;
                bool fine = true;
                for ( int i = 0; i < count && pos < PlainLength && fine; ++i )
                {
                    fine = compare_decrypted_vs_plain( plain, decrypted, pos, block_size );
                    if ( fine )
                        KOutMsg( "[%,u].[%,u] -> correct\n", pos, block_size );
                    else
                        KOutMsg( "[%,u].[%,u] -> failed \n", pos, block_size );
                    pos += block_inc;
                    plain.clear();
                }
                res = fine;
                plain.close();
            }
            KFileRelease( decrypted );
        }
        return res;
    }


    bool decrypted_size_of_file( uint64_t * size )
    {
        const KFile * decrypted;
        bool res = make_decrypted_read_only_file( &decrypted );
        if ( res )
        {
            rc_t rc = KFileSize ( decrypted, size );
            res = ( rc == 0 );
            KFileRelease( decrypted );
        }
        return res;
    }


    bool read_decrypted_bytes( uint64_t pos, unsigned int to_read, unsigned int * read )
    {
        const KFile * decrypted;
        bool res = make_decrypted_read_only_file( &decrypted );
        if ( res )
        {
            char * buffer = ( char * )malloc( to_read );
            if ( buffer == NULL )
                FAIL( "cannot allocate buffer" );
            else
            {
                size_t num_read;
                rc_t rc = KFileReadAll( decrypted, pos, buffer, to_read, &num_read );
                if ( rc != 0 )
                    FAIL( "cannot read from decrypted file" );
                else
                {
                    *read = num_read;
                    res = true;
                }
                free( buffer );
            }
            KFileRelease( decrypted );
        }
        return res;
    }


    bool compare_plain_stream_vs_encrypted_kfile( ifstream &plain, const KFile *decrypted )
    {
        bool res = false;
        plain.seekg( 0, ios::end );
        if ( !plain.good() )
            FAIL( "cannot seek to end of plain-file in cmp_plain_vs_enc" );
        else
        {
            size_t plain_size = plain.tellg();
            if ( !plain.good() )
                FAIL( "cannot query size of plain-file in cmp_plain_vs_enc" );
            else
            {
                plain.seekg( 0, ios::beg );
                if ( !plain.good() )
                    FAIL( "cannot seek to begin of plain-file in cmp_plain_vs_enc" );
                else
                {
                    uint64_t decrypted_size = 0;
                    rc_t rc = KFileSize ( decrypted, &decrypted_size );
                    if ( rc != 0 )
                        FAIL( "cannot query size of encrypted file in cmp_plain_vs_enc" );
                    else
                    {
                        if ( plain_size != decrypted_size )
                            FAIL( "size of plaintext and decrypted text differ in cmp_plain_vs_enc" );
                        else
                        {
                            char * plain_buffer = ( char * ) malloc( plain_size );
                            if ( plain_buffer == NULL )
                                FAIL( "cannot allocate buffer for plain-file in cmp_plain_vs_enc" );
                            else
                            {
                                plain.read( plain_buffer, plain_size );
                                if ( !plain.good() )
                                    FAIL( "cannot read from plain-file in cmp_plain_vs_enc" );
                                else
                                {
                                    char * decrypted_buffer = ( char * ) malloc( decrypted_size );
                                    if ( decrypted_buffer == NULL )
                                        FAIL( "cannot allocate buffer for decrypted file in cmp_plain_vs_enc" );
                                    else
                                    {
                                        size_t num_read;
                                        rc = KFileReadAll ( decrypted, 0,  decrypted_buffer, decrypted_size, &num_read );
                                        if ( rc != 0 )
                                            FAIL( "cannot read from decrypted file in cmp_plain_vs_enc" );
                                        else
                                            res = ( memcmp( plain_buffer, decrypted_buffer, plain_size ) == 0 );
                                        free( decrypted_buffer );
                                    }
                                }
                                free( plain_buffer );
                            }
                        }
                    }
                }
            }
        }
        return res;
    }


    bool make_decrypted_read_only_file( const KFile ** decrypted )
    {
        bool res = false;
        const KFile * wrapped;
        rc_t rc = KDirectoryOpenFileRead( wd, &wrapped, "%s", EncName.c_str() );
        if ( rc != 0 )
            FAIL( "cannot open encrypted file (rd only)" );
        else
        {
            if ( verify )
            {
                rc = KEncFileValidate ( wrapped );
                if ( rc != 0 )
                    KOutMsg( "verify of encrypted file failed\n" );
            }
            if ( rc == 0 )
            {
                const KFile * dec_src = NULL;
                if ( pre_decrypt_buffer > 0 )
                {
                    rc = KBufReadFileMakeRead ( &dec_src, wrapped, pre_decrypt_buffer );
                    if ( rc != 0 )
                        KOutMsg( "cannot build pre-decrypt buffering\n" );
                }
                else
                {
                    dec_src = wrapped;
                    rc = KFileAddRef( dec_src );
                    if ( rc != 0 )
                        KOutMsg( "cannot AddRef org. file\n" );
                }
                if ( rc == 0 )
                {
                    const KFile * dec = NULL;
                    rc = KEncFileMakeRead( &dec, dec_src, &key );
                    if ( rc != 0 )
                        FAIL( "cannot wrap encrypted file with decryption" );
                    else
                    {
                        if ( post_decrypt_buffer > 0 )
                        {
                            rc = KBufReadFileMakeRead ( decrypted, dec, post_decrypt_buffer );
                            if ( rc != 0 )
                                KOutMsg( "cannot build post-decrypt buffering\n" );
                            else
                                KFileRelease( dec );
                        }
                        else
                            *decrypted = dec;

                        res = ( rc == 0 );
                    }
                    KFileRelease( dec_src );
                }
            }
            KFileRelease( wrapped );
        }
        return res;
    }


    bool make_decrypted_read_write_file( KFile ** decrypted )
    {
        bool res = false;
        KFile * wrapped;
        rc_t rc = KDirectoryOpenFileWrite ( wd, &wrapped, true, "%s", EncName.c_str() );
        if ( rc != 0 )
            FAIL( "cannot open encrypted file (rd/wr)" );
        else
        {
            if ( verify )
            {
                rc = KEncFileValidate ( wrapped );
                if ( rc != 0 )
                    KOutMsg( "verify of encrypted file failed\n" );
            }
            if ( rc == 0 )
            {
                KFile * dec_src = NULL;
                if ( pre_decrypt_buffer > 0 )
                {
                    rc = KBufFileMakeWrite ( &dec_src, wrapped, true, pre_decrypt_buffer );
                    if ( rc != 0 )
                        KOutMsg( "cannot build pre-decrypt buffering\n" );
                }
                else
                {
                    dec_src = wrapped;
                    rc = KFileAddRef( dec_src );
                    if ( rc != 0 )
                        KOutMsg( "cannot AddRef org. file\n" );
                }
                if ( rc == 0 )
                {
                    KFile * dec = NULL;
                    rc = KEncFileMakeUpdate( &dec, dec_src, &key );
                    if ( rc != 0 )
                        FAIL( "cannot wrap encrypted file with decryption" );
                    else
                    {
                        if ( post_decrypt_buffer > 0 )
                        {
                            rc = KBufFileMakeWrite ( decrypted, dec, true, post_decrypt_buffer );
                            if ( rc != 0 )
                                KOutMsg( "cannot build post-decrypt buffering\n" );
                            else
                                KFileRelease( dec );
                        }
                        else
                            *decrypted = dec;

                        res = ( rc == 0 );
                    }
                    KFileRelease( dec_src );
                }
            }
            KFileRelease( wrapped );
        }
        return res;
    }


    bool fill_with_value( KFile * f, uint64_t len, char value )
    {
        bool res = true;
        char buffer[ 1 ];
        uint64_t pos = 0;
        buffer[ 0 ] = value;
        for ( pos = 0; pos < len && res; ++pos )
        {
            size_t num_writ;
            rc_t rc = KFileWrite ( f, pos, ( const void * )buffer, 1, &num_writ );
            res = ( rc == 0 && num_writ == 1 );
        }
        return res;
    }


    bool check_for_value( KFile * f, uint64_t len, char value )
    {
        bool res = true;
        char buffer[ 1 ];
        uint64_t pos = 0;
        for ( pos = 0; pos < len && res; ++pos )
        {
            size_t num_read;
            rc_t rc = KFileRead ( f, pos, ( void * )buffer, 1, &num_read );
            res = ( rc == 0 && num_read == 1 && buffer[ 0 ] == value );
        }
        return res;
    }


    bool compare_plain_vs_encrypted( void )
    {
        const KFile * decrypted = NULL;
        bool res = make_decrypted_read_only_file( &decrypted );
        if ( res )
        {
            ifstream plain( PlainName.c_str(), ifstream::in | ifstream::binary );
            if ( plain == NULL )
                FAIL( "cannot open plaintext-file for comparison" );
            else
            {
                res = compare_plain_stream_vs_encrypted_kfile( plain, decrypted );
                plain.close();
            }
            KFileRelease( decrypted );
        }
        return res;
    }


    bool set_file_size( const char * filename, uint64_t new_size )
    {
        rc_t rc = KDirectorySetFileSize ( wd, new_size, "%s", filename );
        if ( rc != 0 )
            FAIL( "cannot change file size" );
        return ( rc == 0 );
    }


    bool alter_byte( const char * filename, size_t pos, unsigned char mask )
    {
        bool res = false;
        fstream fstr( filename, fstream::in | fstream::out | fstream::binary );
        if ( fstr == NULL )
            FAIL( "cannot open file to alter bits" );
        else
        {
            fstr.seekg( pos );
            if ( !fstr.good() )
                FAIL( "cannot seek to alter bits" );
            else
            {
                char value;
                fstr.read( &value, 1 );
                if ( !fstr.good() )
                    FAIL( "cannot read byte to alter bits" );
                else
                {
                    value ^= mask;
                    fstr.seekg( pos );
                    if ( !fstr.good() )
                        FAIL( "cannot seek to alter bits" );
                    else
                    {
                        fstr.write( (const char *)&value, 1 );
                        if ( !fstr.good() )
                            FAIL( "cannot write byte to alter bits" );
                    }
                }
            }
            fstr.close();
        }
        return res;
    }


    bool append_block( const char * filename, const char *data, size_t len )
    {
        bool res = false;
        fstream fstr( filename, fstream::in | fstream::out | fstream::ate | fstream::binary );
        if ( fstr == NULL )
            FAIL( "cannot open file to append data" );
        else
        {
            fstr.write( data, len );
            res = fstr.good();
            if ( !res )
                FAIL( "cannot append data" );
            fstr.close();
        }
        return res;
    }


    size_t size_of_file( const char * filename )
    {
        size_t res = 0;
        fstream fstr( filename, fstream::in | fstream::out | fstream::binary );
        if ( fstr == NULL )
            FAIL( "cannot open file to detect size" );
        else
        {
            fstr.seekg( 0, ios::end );
            if ( !fstr.good() )
                FAIL( "cannot seek to detect size" );
            else
                res = fstr.tellg();
            fstr.close();
        }
        return res;
    }


    bool random_write_values( KFile * f, uint64_t len, uint64_t writes )
    {
        bool res = false;
        KVector *v;
        rc_t rc = KVectorMake ( &v );
        if ( rc != 0 )
            FAIL( "cannot create KVector" );
        else
        {
            // add a boolean for every possible position
            for ( uint64_t i = 0; i < len && rc == 0; ++i )
                rc = KVectorSetBool ( v, i, false );
            if ( rc != 0 )
                FAIL( "cannot create KVector" );
            uint64_t pos_handled = 0;
            while ( pos_handled < writes && rc == 0 )
            {
                uint64_t pos = rand() % len;
                bool is_set;
                rc = KVectorGetBool ( v, pos, &is_set );
                if ( rc == 0 && !is_set )
                {
                    char value = ( ( pos / 7 ) & 0xFF );
                    size_t num_writ;
                    rc = KFileWrite ( f, pos, ( const void * )&value, 1, &num_writ );
                    if ( rc == 0 && num_writ == 1 )
                    {
                        rc = KVectorSetBool ( v, pos, true );
                        if ( rc == 0 )
                            pos_handled++;
                    }
                }
            }
            res = ( rc == 0 && pos_handled == writes );
            KVectorRelease( v );
        }
        return res;
    }
};

int test_nr = 0;

FIXTURE_TEST_CASE( test_1_random_and_continous_access, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : random and continous access, 1MB \n", ++test_nr );
    set_PlainLength( 1000000 );
    REQUIRE( initialize() );

    int sample_count = 10;
    REQUIRE( random_access_decryption( sample_count, true ) ); /* true --> read_only */

    int start_pos = 0;
    int block_inc = 5000;
    int block_size = 5000;
    REQUIRE( continous_decryption( start_pos, block_inc, sample_count, block_size ) );

    block_inc = 7000;
    REQUIRE( continous_decryption( start_pos, block_inc, sample_count, block_size ) );

    start_pos = PlainLength - block_size;
    REQUIRE( continous_decryption( start_pos, -block_inc, sample_count, block_size ) );

    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( test_2_small_file, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : random access, 100 bytes \n", ++test_nr );
    set_PlainLength( 100 );
    REQUIRE( initialize() );

    // perform random decryptions and comparisons */
    int sample_count = 20;
    REQUIRE( random_access_decryption( sample_count, true ) ); /* true --> read_only */

    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( empty_file, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : empty plaintext-file \n", ++test_nr );

    set_PlainLength( 0 );
    REQUIRE( initialize() );

    uint64_t decrypted_size;
    REQUIRE( decrypted_size_of_file( &decrypted_size ) );
    REQUIRE( decrypted_size == 0 );

    unsigned int read;
    REQUIRE( read_decrypted_bytes( 0, 100, &read ) );
    REQUIRE( read == 0 );

    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( pwd_length_1, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : password length 1 \n", ++test_nr );

    set_PwdLength( 1 );
    set_Verify( true );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( pwd_length_4k, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : password length 4k \n", ++test_nr );

    set_PwdLength( 4096 );
    set_Verify( true );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( aes128, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : with AES128 \n", ++test_nr );
    set_KeyType( kkeyAES128 );
    set_Verify( true );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( aes192, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : with AES191 \n", ++test_nr );
    set_KeyType( kkeyAES192 );
    set_Verify( true );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( aes256, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : with AES256 \n", ++test_nr );
    set_KeyType( kkeyAES256 );
    set_Verify( true );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( truncated, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : encrypted file is truncated\n", ++test_nr );

    set_Verify( true );
    REQUIRE( initialize() );
    REQUIRE( set_file_size( EncName.c_str(), encrypted_size( PlainLength ) - 1000 ) );
    REQUIRE( !compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( bits_flipped, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : bits flipped\n", ++test_nr );

    set_Verify( true );
    REQUIRE( initialize() );
    alter_byte( EncName.c_str(), encrypted_size( PlainLength ) - 2000, 0xA5 );
    REQUIRE( !compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( data_appended, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : data appended\n", ++test_nr );

    set_Verify( true );
    REQUIRE( initialize() );
    const char block[ 5 ] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    REQUIRE( append_block( EncName.c_str(), block, 5  ) );
    REQUIRE( !compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( zero_pattern, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : zero-pattern\n", ++test_nr );

    set_Pattern( "\000", 1 );
    set_Verify( true );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( bin_pattern, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : bin-pattern 0x55,0xAA\n", ++test_nr );

    const char pattern[ 2 ] = { 0x55, 0xAA };
    set_Pattern( pattern, 2 );
    set_Verify( true );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( pre_decrypt_buffering, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : pre-decrypt-buffering\n", ++test_nr );

    set_Verify( true );
    set_PreDecBuffer( 32000 );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( post_decrypt_buffering, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : pre-decrypt-buffering\n", ++test_nr );

    set_Verify( true );
    set_PostDecBuffer( 32000 );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( pre_and_post_decrypt_buffering, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : pre-and-post-decrypt-buffering\n", ++test_nr );

    set_Verify( true );
    set_PreDecBuffer( 32000 );
    set_PostDecBuffer( 32000 );
    REQUIRE( initialize() );
    REQUIRE( compare_plain_vs_encrypted() );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( post_decrypt_buffering_random, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : pre-decrypt-buffering random access\n", ++test_nr );

    set_Verify( true );
    set_PostDecBuffer( 60000 );
//    set_PreDecBuffer( 60000 );
    set_PlainLength( 1000000 );
    REQUIRE( initialize() );

    int sample_count = 10;
    REQUIRE( random_access_decryption( sample_count, false ) ); /* true --> read_only */

    int start_pos = 0;
    int block_inc = 5000;
    int block_size = 5000;
    REQUIRE( continous_decryption( start_pos, block_inc, sample_count, block_size ) );

    block_inc = 7000;
    REQUIRE( continous_decryption( start_pos, block_inc, sample_count, block_size ) );

    start_pos = PlainLength - block_size;
    REQUIRE( continous_decryption( start_pos, -block_inc, sample_count, block_size ) );

    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( write_to_encrypted_file, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : write to and read from encrypted file\n", ++test_nr );

    set_PlainLength( 100000 );
    REQUIRE( initialize() );

    KFile *encrypted;
    REQUIRE( make_decrypted_read_write_file( &encrypted ) );
    REQUIRE( fill_with_value( encrypted, PlainLength, 0 ) );
    REQUIRE( check_for_value( encrypted, PlainLength, 0 ) );
    REQUIRE_RC( KFileRelease( encrypted ) );
    KOutMsg( "success!\n" );
}


FIXTURE_TEST_CASE( random_writes_to_encrypted_file, CryptoFixture )
{
    KOutMsg( "\n---------------------------------------------------------------------\n" );
    KOutMsg( "test %u : random writes to encrypted file\n", ++test_nr );

    set_PlainLength( 100000 );
//    set_PostDecBuffer( 1000 );
    REQUIRE( initialize() );

    KFile *encrypted;
    REQUIRE( make_decrypted_read_write_file( &encrypted ) );
    REQUIRE( random_write_values( encrypted, PlainLength, 1 ) );
    REQUIRE_RC( KFileRelease( encrypted ) );
    KOutMsg( "success!\n" );
}


//////////////////////////////////////////// Main

extern "C"
{

    ver_t CC KAppVersion( void )
    {
        return 0x1000000;
    }

    rc_t CC UsageSummary( const char * prog_name )
    {
        return 0;
    }

    rc_t CC Usage( const Args * args )
    {
        return 0;
    }

    const char UsageDefaultName[] = "krypto-test";

    rc_t CC KMain( int argc, char *argv [] )
    {
        rc_t rc = KryptoTestSuite( argc, argv );
        return rc;
    }

}
