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
use File::Spec;

if ( @ARGV < 1 )
{
  print "Usage:\n";
  print "  bam-diff1.pl <DIR>\n\n";
  print "Summary:\n";
  print "  Find the files created by scripts sam-tools-dump.pl and\n";
  print "  vdb-tools-dump.pl and display any differences.\n\n";
  print "Parameters:\n";
  print "  <DIR> is the directory to start our work in.\n";
  print "    This directory must already exist and be populated.\n\n";
  exit ( -1 );
}


# options and parameters
my $indir = $ARGV[ 0 ];

# validate directory structure
unless ( -d $indir )
{
  print( "\tOUT-DIR parameter not a usable directory\n" );
  exit ( 3 );
}

my @required_directories = ( "$indir/samtools",
                             "$indir/vdbtools",
                             "$indir/samtools/aligned",
                             "$indir/vdbtools/aligned",
                             "$indir/samtools/unaligned",
                             "$indir/vdbtools/unaligned" );

foreach ( @required_directories )
{
  if ( ! -d $_ )
  {
    print ( "\tMissing directory '$_', aborting...\n\n" );
    exit ( 3 );
  }
}

# build a list of the files in each of the four sub-directories
my @samtool_aligned_core_files = glob( "$indir/samtools/aligned/core_*" );
my @samtool_aligned_ext_files = glob( "$indir/samtools/aligned/ext_*" );
my @samtool_unaligned_files = glob( "$indir/samtools/unaligned/*" );

my @vdb_aligned_core_files = glob( "$indir/vdbtools/aligned/core_*" );
my @vdb_aligned_ext_files = glob( "$indir/vdbtools/aligned/ext_*" );
my @vdb_unaligned_files = glob( "$indir/vdbtools/unaligned/*" );

if ( ( $#samtool_aligned_core_files != $#vdb_aligned_core_files ) ||
     ( $#samtool_aligned_core_files != $#samtool_aligned_ext_files ) ||
     ( $#samtool_aligned_core_files != $#vdb_aligned_ext_files ) ||
     ( $#samtool_unaligned_files != $#vdb_unaligned_files ) )
{
    print( "\tsamtool_aligned_core_files: $#samtool_aligned_core_files\n" );
    print( "\tvdb_aligned_core_files    : $#vdb_aligned_core_files\n" );
    print( "\tsamtool_aligned_ext_files : $#samtool_aligned_ext_files\n" );
    print( "\tvdb_aligned_ext_files     : $#vdb_aligned_ext_files\n" );
    print( "\tsamtool_unaligned_files   : $#samtool_unaligned_files\n" );
    print( "\tvdb_unaligned_files       : $#vdb_unaligned_files\n" );
    print( "\tBad dump files - counts mismatch\n" );
    exit ( 3 );
}


my $diffdir = "$indir/diffs";
my $error_count = 0;

mkdir $diffdir;
mkdir "$diffdir/aligned";
mkdir "$diffdir/unaligned";

# look for missing files and put out a message for files
# produced by one side but not the other
foreach $path ( @samtool_aligned_core_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $path );
  my $to_look_for = "$indir/vdbtools/aligned/$file";
  if ( ! -f $to_look_for )
  {
    print( "\tERROR: missing file '$to_look_for'\n" );
    ++$error_count;
  }
}

foreach $path ( @samtool_aligned_ext_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $path );
  my $to_look_for = "$indir/vdbtools/aligned/$file";
  if ( ! -f $to_look_for )
  {
    print( "\tERROR: missing file '$to_look_for'\n" );
    ++$error_count;
  }
}

foreach $path ( @samtool_unaligned_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $path );
  my $to_look_for = "$indir/vdbtools/unaligned/$file";
  if ( ! -f $to_look_for )
  {
    print( "\tERROR: missing file '$to_look_for'\n" );
    ++$error_count;
  }
}

foreach $path ( @vdb_aligned_core_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $path );
  my $to_look_for = "$indir/samtools/aligned/$file";
  if ( ! -f $to_look_for )
  {
    print( "\tERROR: missing file '$to_look_for'\n" );
    ++$error_count;
  }
}

foreach $path ( @vdb_aligned_ext_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $path );
  my $to_look_for = "$indir/samtools/aligned/$file";
  if ( ! -f $to_look_for )
  {
    print( "\tERROR: missing file '$to_look_for'\n" );
    ++$error_count;
  }
}

foreach $path ( @vdb_unaligned_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $path );
  my $to_look_for = "$indir/samtools/unaligned/$file";
  if ( ! -f $to_look_for )
  {
    log_it ( "ERROR: missing file '$to_look_for'\n" );
    ++$error_count;
  }
}

my $cnt_match = 0;

# compare files that are in both the samtools and vdb tools directories
# identical files produce no output
foreach $sam_path ( @samtool_aligned_core_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $sam_path );
  my $vdb_path = "$indir/vdbtools/aligned/$file";
  if ( -f $vdb_path )
  {
    if ( system ( "diff $sam_path $vdb_path >$diffdir/$file.diffs" ) == 0 )
    {
       ++$cnt_match;
       unlink ( "$diffdir/$file.diffs" );
    }
    else
    {
       ++$error_count;
    }
  }
}

foreach $sam_path ( @samtool_aligned_ext_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $sam_path );
  my $vdb_path = "$indir/vdbtools/aligned/$file";
  if ( -f $vdb_path )
  {
    if ( system ( "ext_diff.pl $sam_path $vdb_path >$diffdir/$file.diffs" ) == 0 )
    {
      ++$cnt_match;
      unlink ( "$diffdir/$file.diffs" );
    }
    else
    {
      ++$error_count;
    }
  }
}

foreach $sam_path ( @samtool_unaligned_files )
{
  my ( $vol, $dirs, $file ) = File::Spec->splitpath( $sam_path );
  my $vdb_path = "$indir/vdbtools/unaligned/$file";
  if ( -f $vdb_path )
  {
    if ( system ( "diff $sam_path $vdb_path >$diffdir/$file.diffs" ) == 0 )
    {
      ++$cnt_match;
      unlink ( "$diffdir/$file.diffs" );
    }
    else
    {
      ++$error_count;
    }
  }
}

print( "\n\t############## $cnt_match files match, $error_count files mismatch ##############\n" );
exit ( $error_count );
