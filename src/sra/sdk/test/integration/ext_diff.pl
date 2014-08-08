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

#----------------------------------------------------------
# this script performes a special-comparison of ext-files
#----------------------------------------------------------

my $nargs = scalar @ARGV;

if ( $nargs == 0  )
{
    print( "\n-------------------------------------------------------------------------\n" );
    print( "USAGE: ext_diff.pl dump1 dump2\n" );
    print( "dump1 ... the full path of dump-file #1\n" );
    print( "dump2 ... the full path of dump-file #2\n" );
    print( "output is produced to console... \n" );
    print( "-------------------------------------------------------------------------\n\n" );
}
else
{
    my $dump1 = $ARGV[ 0 ];
    my $dump2 = $ARGV[ 1 ];

    if ( ! -f $dump1 )
        { die "$dump1 does not exist!\n" } 
    if ( ! -f $dump2 )
        { die "$dump2 does not exist!\n" } 

    print ( "comparing $dump1 vs. $dump2\n" );

    open ( DUMP1, "<", "$dump1" ) or die "open $dump1 failed";
    open ( DUMP2, "<", "$dump2" ) or die "open $dump2 failed";

    my( $row1, $row2, $line, $diffcount );
    $line_nr = 0;
    $diffcount = 0;
    while ( ( $row1 = <DUMP1> ), defined( $row1 ) )
    {
        $row2 = <DUMP2>;
        my $diffs = 0;

        if ( ! defined ($row2))
        {
            print ( "line missing file 2\n" );
            $diffcount++;
        }
        elsif ( $row1 ne $row2 )
        {
            chomp $row1;
            chomp $row2;

            my @cols1 = split( /\t/, $row1 );
            my @cols2 = split( /\t/, $row2 );

            if ( $#cols1 != $#cols2 )
            {
                print( "line $line_nr :\n" );
                $diffs++;
            }
            elsif ( $#cols1 != 3 )
            {
                print( "line $line_nr : not 4 columns\n" );
                $diffs++;
            }
            elsif ( $cols1[ 0 ] ne $cols2[ 0 ] )
            {
                print( "line $line_nr : column 1 differs\n" );
                $diffs++;
            }
            elsif ( $cols1[ 1 ] ne $cols2[ 1 ] )
            {
                my $diff = $cols1[ 1 ] - $cols2[ 1 ];
                if ( $diff > 1 || $diff < -1 )
                {
                    print( "line $line_nr : column 2 differs\n" );
                    $diffs++;
                }
            }
            elsif ( $cols1[ 2 ] ne $cols2[ 2 ] )
            {
                print( "line $line_nr : column 3 differs\n" );
                $diffs++;
            }
            elsif ( $cols1[ 3 ] ne $cols2[ 3 ] )
            {
                print( "line $line_nr : column 4 differs\n" );
                $diffs++;
            }
            if ($diffs != 0)
            {
                $diffcount++;
                print (">$row1<\n>$row2<\n\n");
            }
        }
        $line_nr++;
    }
    while (($row2 = <DUMP2>), defined ($row2))
    {
        print ( "line missing file 1\n" );
        $diffcount++;
        $line_nr++;
    }
    close ( DUMP1 );
    close ( DUMP2 );

    my $s_percent = "?";
    if ( $line_nr > 0 )
    {
        my $percent = ( $diffcount * 100 ) / $line_nr;
        $s_percent = sprintf( "%.2f %%", $percent );
    }
    print( "$diffcount different lines found in $line_nr lines ($s_percent)\n" );

    exit ( $diffcount );
}
