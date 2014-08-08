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
my $filename = $ARGV[0];
my $filepos  = $ARGV[1];
my $pattern  = $ARGV[2];

if ( not defined( $filename ) ) { die "no filename" };
if ( not defined( $filepos ) )  { $filepos = 0; };
if ( not defined( $pattern ) )  { $pattern = 0; };

print( "\nchange a value in a file\n" );
print( "file     $filename\n" );
print( "pos      $filepos\n" );
print( "value    $pattern\n" );

open my $fh, '+<', $filename  or die "open failed: $!\n";
binmode $fh;
seek( $fh, $filepos, 0 ) == 1 or die "seek failed: $!\n";
my $buffer='';
vec( $buffer, 0, 8 ) = int( $pattern );
syswrite( $fh, $buffer, 1 ) == 1 or die "write failed: $!\n";
close $fh                      or die "close failed: $!\n"; 