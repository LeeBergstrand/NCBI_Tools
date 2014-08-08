#! /usr/bin/perl -w
use File::Path qw( mkpath rmtree );
use File::Basename;
use File::Spec;

sub exec_tool
{
    my ( $cmd, $toolpath ) = @_;
    my $ret = "";
    my $row;

    if ( $^O eq "MSWin32" )
    {
        $toolpath .= "\\";
    }
    open ( CMD, "-|", $toolpath . $cmd ) or die "$cmd failed";
    while ( ( $row = <CMD> ), defined( $row ) )
    {
        $row =~ s/\r?\n$//;
        $ret .= $row;
    }
    close ( CMD );
    return $ret;
}


sub get_tool_vers
{
    my ( $toolpath ) = @_;
    my $ret = exec_tool( "vdb-dump --version", $toolpath );

    # return everything after the last match, that means only the version...
    if ( $^O eq "cygwin" )
    {
        $ret =~ m/vdb-dump.exe : /;
        return $';
    }
    else
    {
        $ret =~ m/vdb-dump : /;
        return $';
    }
}


sub get_accession_path
{
    my ( $src, $toolpath ) = @_;
    return exec_tool( "srapath $src", $toolpath );
}


sub measure_time
{
    my ( $cmd ) = @_;
    my $start = time;
    my $res = system( $cmd );
    my $duration = time - $start;
    print( "execution-time = $duration s\n" );
    return $res;
}


sub abi_dump
{
    my ( $src, $start, $end, $dst, $toolpath ) = @_;
    if ( $^O eq "MSWin32" )
    {
        $toolpath .= "\\";
    }
    my $abi_dump_dst = File::Spec->catdir( $dst, 'abi-dump' );
    mkpath( $abi_dump_dst );
    my $log = File::Spec->catfile( $dst, 'abi-dump.log' );
    my $path = get_accession_path( $src, $toolpath );
    my $cmd = $toolpath . "abi-dump -path $path -N $start -X $end -O $abi_dump_dst $src >>$log 2>&1";
    print( "\n$cmd\n" );
    return measure_time( $cmd );
}


sub sra_dump
{
    my ( $src, $start, $end, $dst, $toolpath ) = @_;
    if ( $^O eq "MSWin32" )
    {
        $toolpath .= "\\";
    }
    my $sra_dump_dst = File::Spec->catdir( $dst, 'sra-dump' );
    mkpath( $sra_dump_dst );
    my $out = File::Spec->catfile( $sra_dump_dst, "$src.txt" );
    my $log = File::Spec->catfile( $dst, 'sra-dump.log' );
    my $cmd = $toolpath . "sra-dump --start $start --stop $end $src >$out 2>$log";
    print( "\n$cmd\n" );
    return measure_time( $cmd );
}


sub vdb_dump
{
    my ( $src, $start, $end, $dst, $toolpath ) = @_;
    if ( $^O eq "MSWin32" )
    {
        $toolpath .= "\\";
    }
    my $vdb_dump_dst = File::Spec->catdir( $dst, 'vdb-dump' );
    mkpath( $vdb_dump_dst );
    my $out = File::Spec->catfile( $vdb_dump_dst, "$src.txt" );
    my $log = File::Spec->catfile( $dst, 'vdb-dump.log' );
    my $cmd = $toolpath . "vdb-dump $src -R $start-$end >$out 2>$log";
    print( "\n$cmd\n" );
    return measure_time( $cmd );
}

sub illumina_dump
{
    my ( $src, $start, $end, $dst, $toolpath ) = @_;
    if ( $^O eq "MSWin32" )
    {
        $toolpath .= "\\";
    }
    my $ill_dump_dst = File::Spec->catdir( $dst, 'illumina-dump' );
    mkpath( $ill_dump_dst );
    my $log = File::Spec->catfile( $dst, 'illumina-dump.log' );
    my $cmd = $toolpath . "illumina-dump -N $start -X $end -O $ill_dump_dst $src >>$log 2>&1";
    print( "\n$cmd\n" );
    return measure_time( $cmd );
}

sub fastq_dump
{
    my ( $src, $start, $end, $dst, $toolpath ) = @_;
    if ( $^O eq "MSWin32" )
    {
        $toolpath .= "\\";
    }
    my $fq_dump_dst = File::Spec->catdir( $dst, 'fastq-dump' );
    mkpath( $fq_dump_dst );
    my $log = File::Spec->catfile( $dst, 'fastq-dump.log' );
    my $cmd = $toolpath . "fastq-dump -N $start -X $end -O $fq_dump_dst $src >>$log 2>&1";
    print( "\n$cmd\n" );
    return measure_time( $cmd );
}


my $outdir = $ARGV[ 0 ];
my $toolpath = $ARGV[ 1 ];

if ( !defined $outdir )
{
    print( "\nYou have to specify an existing output directory!\n" );
    print( "\texample: 'dumpto /panfs/pan1/sra-test/username/dumpout'\n\n" );
    exit ( -1 );
}

if ( ! -d $outdir )
{
    print( "\nThe output directory '$outdir' does not exist!\n" );
    exit ( -1 );
}

if ( ! $^O eq "cygwin" )
{
    if ( ! -w $outdir )
    {
        print( "\nThe output directory '$outdir' is not writable!\n" );
        exit ( -1 );
    }
}


#let the File::Spec 'class' select the right path separator for the current OS!
my $complete_out = File::Spec->catdir( $outdir, get_tool_vers( $toolpath ), $^O );

print( "\ndumping to '$complete_out'\n\n" );

if ( -d $complete_out )
{
    File::Path->rmtree( [ $complete_out ] );
}

if ( ! -d $complete_out )
{
    print ( "\n ... lets make '$complete_out'\n" );
    make_path( $complete_out );
}

if ( ! -d $complete_out )
{
    print( "\nunable the create '$complete_out'!\n" );
} 

my $err = 0;
my $ok = 0;

#perform the abi-dump...
if ( abi_dump( "SRR000001", 1, 300, $complete_out, $toolpath ) == 0 ) { ++$ok; } else { ++$err; }

#perform the sra-dump...
#if ( sra_dump( "SRR000001", 1, 300, $complete_out, $toolpath ) == 0 ) { ++$ok; } else { ++$err; }

#perform the vdb-dump...
if ( vdb_dump( "SRR000001", 1, 300, $complete_out, $toolpath ) == 0 ) { ++$ok; } else { ++$err; }

#perform the illumina-dump...
if ( illumina_dump( "DRR000001", 1, 300, $complete_out, $toolpath ) == 0 ) { ++$ok; } else { ++$err; }

#perform the fastq-dump...
if ( fastq_dump( "SRR000001", 1, 300, $complete_out, $toolpath ) == 0 ) { ++$ok; } else { ++$err; }

print( "\n$ok dumps successful / $err dumps failed\n" );
