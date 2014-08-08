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
use Fcntl;
use File::Path;
use File::Basename;
use Time::Local;

#----------------------------------------------------------
# this is the master-perl-script for BAM-load-verification
#----------------------------------------------------------

if ( @ARGV < 3 )
{
    print("\n-------------------------------------------------------------------------\n");
    print("USAGE: validbam1.pl bam-file cfg-file outdir <-cs>\n");
    print("bam-file ... the full path to the source-bam-file\n");
    print("cfg-file ... the full path to the cfg-file (needed to perfrom the load)\n");
    print("outdir   ... opt. output-dir, if not present a pid-based one is used\n");
    print("-------------------------------------------------------------------------\n\n");
    exit ( -1 );
}

my ( $outdir, $bamsrc, $cfg );
$bamsrc = $ARGV[ 0 ];
$cfg    = $ARGV[ 1 ];
$outdir = $ARGV[ 2 ];

if ( -d $outdir )
{
    system( "rm -rf $outdir" );
    mkpath( $outdir ) or die $!;
}

unless ( -f $bamsrc )
{
    print( "$bamsrc can't be found" );
    exit ( -1 );
}

unless ( -f $cfg )
{
    print( "$cfg can't be found" );
    exit ( -1 );
}

my $bamsize = -s $bamsrc;
my $failed = "";


####################################################################
#   forking the sam-tools-script
####################################################################
my $sam_tools_pid = fork();
if ( $sam_tools_pid == 0 )
{
    $failed = "";

    printf( "VALIDBAM: forked SAM-TOOLS-DUMP (%s)\n\tBAM: %s\n\tSIZE: %lu\n\tDIR: %s\n",
             scalar localtime, $bamsrc, $bamsize, $outdir );

    if ( system ( "sam-tools-dump.pl $bamsrc $outdir" ) )
    {
        $failed = " ERROR";
        rmtree ( "$outdir/samtools" );
    }

    printf( "\nVALIDBAM:%s finished SAM-TOOLS-DUMP (%s)\n\tBAM: %s\n\tDIR: %s\n",
            $failed, scalar localtime, $bamsrc, $outdir );

    exit ( ( $failed eq "" ) ? 0 : -1 );
}


####################################################################
#   forking the bam-load and the vdb-scripts...
####################################################################
my $vdb_tools_pid = fork();
if ( $vdb_tools_pid == 0 )
{
    my ( $failed, $tmpfs, $tmpdir );
    $failed = "";
    $tmpfs  = "/export/home/TMP" if -d "/export/home/TMP";
    $tmpfs  = "/tmp" unless $tmpfs;
    $tmpdir = "$tmpfs/validbam.$$";

    mkpath "$tmpdir" or die $!;

    printf( "\nVALIDBAM: started BAM-LOAD (%s)\n\tBAM: %s\n\tDIR: %s\n",
            scalar localtime, $bamsrc, $tmpdir );

####################################################################
#   STEP 1 ... loading the BAM-file with "bam-load"
####################################################################
    my $bam = "bam-load -E 100000 --accept-dups --accept-nomatch --cache-size=".(32*1024)." --tmpfs=$tmpfs -L err -o $tmpdir/vdb -k $cfg $bamsrc";
    printf( "$bam\n" );
    if ( open FH, "$bam |" )
    {
        while ( <FH> )
        {
            print( $_ );
        }
        close FH;
        if ( $? )
        {
            $failed = "ERROR ";
        }
    }
    else
    {
        $failed = "ERROR ";
    }

    printf( "\nVALIDBAM: %sBAM-LOAD done (%s)\n\tBAM: %s\n\tDIR:  %s\n",
             $failed, scalar localtime, $bamsrc, $outdir );

    if ( $failed eq "" )
    {
        if ( -f "$outdir/vdb.csra" )
        {
            system( "rm $outdir/vdb.csra" );
        }
        printf( "\nVALIDBAM: started KAR (%s)\n\tBAM: %s\n\tDIR: %s\n",
                scalar localtime, $bamsrc, $tmpdir );
####################################################################
#   STEP 2 ... transforming the created directory with "kar"
####################################################################
        if ( system ( "kar -c $outdir/vdb.csra -d $tmpdir/vdb" ) != 0 )
        {
            $failed = " ERROR ";
        }
        printf( "\nVALIDBAM: %s KAR done (%s)\n\tBAM:  %s\n\tDIR:  %s\n",
                 $failed, scalar localtime, $bamsrc, $outdir );
    }
    rmtree ( $tmpdir );


    if ( -f "$outdir/vdb.csra" )
    {
        printf( "\nVALIDBAM: started VDB-DUMP-TOOLS (%s)\n\tBAM: %s\n\tDIR: %s\n",
                scalar localtime, "$outdir/vdb.csra", $outdir );

####################################################################
#   STEP 3 ... dumping the csra-file with "vdb-tools-dump1.pl"
####################################################################
        if ( system( "vdb-tools-dump1.pl $outdir/vdb.csra $outdir" ) != 0 )
        {
            $failed = " ERROR ";
        }
        printf( "\nVALIDBAM: VDB-TOOLS-DUMP done (%s) %s\n\tBAM:  %s\n\tDIR:  %s\n",
                scalar localtime, $failed, "$outdir/vdb.csra", $outdir );
    }
    printf( "\nVALIDBAM: %s VDB-TOOLS done (%s)\n\tBAM:  %s\n\tDIR:  %s\n",
             $failed, scalar localtime, $bamsrc, $outdir );

    exit ( ( $failed eq "" ) ? 0 : 3 );
}

####################################################################
#   waiting for the child processes
####################################################################
waitpid( $vdb_tools_pid, 0 );
waitpid( $sam_tools_pid, 0 );

printf( "\nVALIDBAM: started BAM-DIFF1 (%s)\n\tBAM: %s\n\tDIR: %s\n",
         scalar localtime, $bamsrc, $outdir );

if ( system( "bam-diff1.pl $outdir" ) != 0 )
{
    $failed = " ERROR";
}

printf( "\nVALIDBAM:%s done (%s)\n\tBAM:  %s\n\tDIR:  %s\n",
        $failed, scalar localtime, $bamsrc, $outdir );

exit ( $failed eq "" ? 0 : -1 );
