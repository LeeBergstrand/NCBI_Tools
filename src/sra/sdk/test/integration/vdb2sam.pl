#!/usr/bin/perl -w
# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================

use IO::File;

#--------------------------------------------------------------------------------
# recreate the SAM-file-format from a vdb-directory containing a loaded BAM-file
#--------------------------------------------------------------------------------

my $nargs = scalar @ARGV;

if ( $nargs == 0  )
{
    print( "\n-------------------------------------------------------------------------\n" );
    print( "USAGE: vdb_2_sam.pl vdb-db sam-file\n" );
    print( "vdb-db   ... the full path of a vdb-directory containing a loaded bam-file\n" );
    print( "samfile  ... output-file to be created\n" );
    print( "-------------------------------------------------------------------------\n\n" );
}
else
{
    my $vdbsrc = $ARGV[ 0 ];

    my %tables = ();
    detect_tables( \$vdbsrc, \%tables );

    print_headers( \$vdbsrc );

    my $tabname = "PRIMARY_ALIGNMENT";
    if ( exists( $tables{ $tabname } ) )
        { print_lines( \$vdbsrc, \$tabname ); }

    $tabname = "SECONDARY_ALIGNMENT";
    if ( exists( $tables{ $tabname } ) )
        { print_lines( \$vdbsrc, \$tabname ); }
}


####################################################################
#   prints the headers
####################################################################
sub print_headers
{
    my ( $src_ref ) = @_;
    my $row;
    my $hdr = 0;

    open ( DUMP, "-|", "kdbmeta $$src_ref" ) or die "kdbmeta ($$src_ref) failed";
    while ( ( $row = <DUMP> ), defined( $row ) )
    {
        chomp( $row );
        $row =~ s|'||;

        if ( $row eq "<BAM_HEADER>" )
        {
            $hdr = 1;
        }
        elsif ( $row eq "</BAM_HEADER>" )
        {
            $hdr = 0;
        }
        elsif ( $hdr > 0 )
        {
            $row =~ s| +||;
            if ( $row ne "" )
                { print( "$row\n" ); }

            # my $idx;
            # my $field;
            # my $sep = 0;
            # my @cols = split( / +/, $row );
            # foreach $field ( @cols )
            # {
                # if ( $field ne "" )
                # {
                    # if ( $sep > 0 )
                        # { print( "\t" ); }
                    # print( "$field" );
                    # $sep++;
                # }
            # }
            # if ( $sep > 1 )
                # { print( "\n" ); }
        }
    }
    close ( DUMP );
}


####################################################################
#   prints the aligned lines
####################################################################
sub print_lines
{
    my ( $src_ref, $tabname_ref ) = @_;
    my $row;
    my $cols = "SEQ_NAME,SAM_FLAGS,REF_NAME,REF_POS,MAPQ,CIGAR_SHORT,";
    $cols .= "MATE_REF_NAME,MATE_REF_POS,TEMPLATE_LEN,READ,";
    $cols .= "SAM_QUALITY,SPOT_GROUP,EDIT_DISTANCE";
    my $cmd = " vdb-dump $$src_ref -T $$tabname_ref -C $cols -f tab";

    open ( DUMP, "-|", "$cmd" ) or die "vdb-dump ($$src_ref) failed";
    while ( ( $row = <DUMP> ), defined( $row ) )
    {
        chomp( $row );
        my $idx = 0;
        my @cols = split( /\t/, $row );
        for ( $idx = 0; $idx < 11; ++$idx)
        {
            if ( $idx > 0 )
                { print( "\t" ); }
            print( "$cols[ $idx ]" );
        }
        if ( $cols[ 11 ] ne "" )
            { print( "\tRG:Z:$cols[ 11 ]" ); }
        if ( $cols[ 12 ] eq "" )
            { print( "\tCM:i:0\n" ); }
        else
            { print( "\tCM:i:$cols[ 12 ]\n" ); }
    }
    close ( DUMP );
}


####################################################################
#   detects which tables do exist in vdbsrc
####################################################################
sub detect_tables
{
    my ( $dir_ref, $hash_ref ) = @_;
    my ( $enum_row );
    open ( ENUM, "-|", "vdb-dump $$dir_ref -E" ) or die "vdb-dump (enum tables) failed";
    while ( ( $enum_row = <ENUM> ), defined( $enum_row ) )
    {
        chomp( $enum_row );
        my @cols = split( / /, $enum_row );
        if ( $cols[ 0 ] eq "tbl" )
        {
            $$hash_ref{ $cols[2] } = $cols[2];
        }
    }
    close ( ENUM );
}
