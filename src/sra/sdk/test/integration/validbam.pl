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

sub usage
{
    print("\n-------------------------------------------------------------------------\n");
    print("USAGE: validbam.pl bam-file cfg-file outdir <-cs>\n");
    print("bam-file ... the full path to the source-bam-file\n");
    print("cfg-file ... the full path to the cfg-file (needed to perfrom the load)\n");
    print("outdir   ... opt. output-dir, if not present a pid-based one is used\n");
    print("-cs      ... use color-space insted of base-space to compare\n");
    print("-debug   ... write to the output as well as a log file in <outdir>\n");
    print("-------------------------------------------------------------------------\n\n");
}

my $log;
my $debug = "";
sub log_it
{
    my $fmt = shift;
    if (defined $fmt)
    {
        printf ($fmt,@_) if ($debug eq " -debug");
        printf{$log} ($fmt,@_) if (defined $log);
    }
}

my $my_lock = 0;

my $index;
my $cs = "";
my @params;

for ($index = 0; $index <= $#ARGV; ++$index)
{
    if (($ARGV[$index] eq '-debug') || ($ARGV[$index] eq '-DEBUG'))
    {
        $debug = " -debug";
    }
    elsif (($ARGV[$index] eq '-cs') || ($ARGV[$index] eq '-CS'))
    {
        $cs = " -cs";
    }
    else
    {
        push(@params, $ARGV[$index]);
    }
}

my $outdir;

if ( $#params == 2 )
{
    $outdir = $params[2];
}
elsif ( $#params == -1 )
{
    usage ();
    exit ( 0 );
}
elsif ( $#params < 2 )
{
    print ( "too few parameters\n" );
    usage ();
    exit ( -1 );
}
elsif ( $#params > 2 )
{
    print ("too many parameters\n");
    usage();
    exit(-1);
}

if ( -f "$outdir/done" )
{
    exit ( 0 );
}

unless ( -d $outdir )
{
    mkpath( $outdir ) or die $!;
}

open ( $log, ">", "$outdir/validbam.$$.log" ) or die $!;

my $bamsrc = $params[ 0 ];

unless ( -f $bamsrc )
{
    log_it ( "$bamsrc can't be found" );
    die "$bamsrc does not exist!\n";
}

my $my_lockfile = 0;
my $lockfile = "$outdir/busy";

my $now = localtime();
sysopen ( LOCKFH, $lockfile, O_EXCL | O_CREAT | O_RDWR ) or die;
$my_lockfile = 1;
print {LOCKFH} ( "$ENV{'HOSTNAME'} $$ $now\n" );
close ( LOCKFH );


END
{
    if ($my_lockfile)
    {
        unlink $lockfile;
    }
}

my $cfg = $params[ 1 ];

unless ( -f $cfg )
{
    log_it ( "$cfg can't be found" );
    exit ( -1 );
}

my $bamsize = -s $bamsrc;
my $failed = "";

log_it ( "VALIDBAM: started\n\tBAM: %s %lu\n\tCFG: %s\n\tDIR: %s\n\tCS: %s\n",
         $bamsrc, $bamsize, $cfg, $outdir, $cs );

####################################################################
#   forking the sam-tools-script
####################################################################
my $sam_tools_pid = fork();
if ( $sam_tools_pid == 0 )
{
    $failed = "";

    log_it ( "VALIDBAM: forked for SAM-TOOLS-DUMP\n\tBAM: %s %lu\n\tDIR: %s\n\tOptions:%s%s \n",
             $bamsrc,$bamsize,$outdir,$cs,$debug );

    if ( -d "$outdir/samtools" )
    {
        log_it ("existing samtools found\n\n");
    }
    else
    {
        if ( system ( "sam-tools-dump.pl $bamsrc $outdir $cs $debug" ) )
        {
            $failed = " ERROR";
            rmtree ( "$outdir/samtools" );
        }
    }
    log_it( "\nVALIDBAM:%s finished for SAM-TOOLS-DUMP\n\tBAM: %s %lu\n\tDIR: %s\n\n",
            $failed, $bamsrc, $bamsize, $outdir );

    exit ( ($failed eq "" ) ? 0 : -1 );
}


####################################################################
#   forking the bam-load and the vdb-scripts...
####################################################################
my $vdb_tools_pid = fork();
if ( $vdb_tools_pid == 0 )
{
    my $failed = "";
    log_it ( "\nVALIDBAM: forked for VDB-TOOLS\n\tBAM: %s\n\tSize: %lu\n\tDIR: %s\n\n",
             $bamsrc, $bamsize, $outdir );

    if ( -d "$outdir/vdbtools" )
    {
        log_it ( "existing vdbtools found\n\n" );
    }
    else
    {
        my $tmpfs;
        my $tmpdir;

        $tmpfs = "/export/home/TMP" if -d "/export/home/TMP";
        $tmpfs = "/tmp" unless $tmpfs;
        $tmpdir = "$tmpfs/validbam.$$";

        if (-f "$outdir/vdb.csra")
        {
            log_it ("existing vdb.csra found\n");
        }
        else
        {
            mkpath "$tmpdir" or die $!;

            log_it ( "\nVALIDBAM: started BAM-LOAD\nBAM: %s\nSIZE: %lu\nDIR: %s\n\n",
                    $bamsrc, $bamsize, $tmpdir );
            $failed = "";
# if (system ("bam-load --cache-size=".(32*1024)." --tmpfs=$tmpfs  -L err -o $tmpdir/vdb -k $cfg $bamsrc") != 0)
            log_it ( "bam-load --cache-size=".(32*1024)." --tmpfs=$tmpfs -L err -o $tmpdir/vdb -k $cfg $bamsrc" );
            if ( open FH, "bam-load --cache-size=".(32*1024)." --tmpfs=$tmpfs -L err -o $tmpdir/vdb -k $cfg $bamsrc |" )
            {
                while ( <FH> )
                {
                    log_it ($_);
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

            log_it ( "VALIDBAM: %sBAM-LOAD done\nBAM:  %s %lu\nDIR:  %s\n",
                     $failed, $bamsrc, $bamsize, $outdir );

            if ( $failed eq "" )
            {
                log_it ( "VALIDBAM: started KAR\nBAM: %s %lu\nDIR: %s\n",
                         $bamsrc,$bamsize,$tmpdir );
                if ( system ( "kar -c $outdir/vdb.csra -d $tmpdir/vdb" ) != 0 )
                {
                    $failed = " ERROR";
                }
                log_it ( "VALIDBAM: %sKAR done\nBAM:  %s %lu\nDIR:  %s\n",
                         $failed, $bamsrc, $bamsize, $outdir );
            }
            rmtree ( $tmpdir );
        }

        if ( -f "$outdir/vdb.csra" )
        {
            unless ( -d $tmpdir )
            {
                mkpath "$tmpdir" or die $!;
            }

            log_it ( "VALIDBAM: started VDB-DUMP-TOOLS\nBAM: %s %lu\nDIR: %s\n",
                     $bamsrc, $bamsize, $tmpdir );

            #extract data from the vdb-database:
            if ( system( "vdb-tools-dump.pl $outdir/vdb.csra $outdir$cs$debug" ) != 0 )
            {
                $failed = " ERROR";
            }
            log_it ( "VALIDBAM: %sVDB-TOOLS-DUMP done at %s \nBAM:  %s\nDIR:  %s\n",
                     $failed, $bamsrc, $outdir );

        }
    }
    log_it ( "VALIDBAM: %sVDB-TOOLS done at\nBAM:  %s %lu\nDIR:  %s\n",
             $failed, $bamsrc, $bamsize, $outdir );
    exit ( ( $failed eq "" ) ? 0 : 3 );
}

####################################################################
#   waiting for the child processes
####################################################################
waitpid( $vdb_tools_pid, 0 );
waitpid( $sam_tools_pid, 0 );

log_it ( "VALIDBAM: started BAM-DIFF\nBAM: %s %lu\nDIR: %s\n",
         $bamsrc, $bamsize, $outdir );

if ( system( "bam-diff.pl $outdir" ) != 0 )
{
    $failed = " ERROR";
}
log_it ( "VALIDBAM:%s done\nBAM:  %s %lu\nDIR:  %s\n",
         $failed, $bamsrc, $bamsize, $outdir );
unlink ( "$outdir/busy" );
exit ( $failed eq "" ? 0 : -1 );

__END__
