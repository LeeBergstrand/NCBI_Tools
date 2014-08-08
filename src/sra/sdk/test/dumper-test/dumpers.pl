#! /usr/bin/perl -w
use File::Path qw( mkpath rmtree );
use File::Basename;
use File::Spec;

my $schedule_file = $ARGV[ 0 ];
my $bin_dir = "";
my $out_dir = "";
my $cmp_ver = "";
my $version = "";

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


sub append_to_file
{
    my ( $filename, $line ) = @_;
    open( THE_FILE, ">>$filename" );
    print( THE_FILE "$line\n" );
    close( THE_FILE );
}


sub measure_time
{
    my ( $cmd, $log ) = @_;
    my $start = time;
    my $res = system( $cmd );
    my $duration = time - $start;
    my $line = "execution-time = $duration s";
    print( "$line\n" );
    append_to_file( $log, $line );
    return $res;
}


sub abi_dump
{
    my ( $src, $start, $end ) = @_;
    my $tool = File::Spec->catdir( $bin_dir, 'abi-dump' );
    my $dst  = File::Spec->catdir( $out_dir, $^O, $version, $src, 'ABI' );
    mkpath( $dst );
    my $log = File::Spec->catfile( $dst, 'abi-dump.log' );
    my $path = get_accession_path( $src, $bin_dir );
    my $cmd = "$tool -path $path -N $start -X $end -O $dst $src >$log 2>&1";
    print( "\nabi-dump $src : $start - $end\n" );
    return measure_time( $cmd, $log );
}


sub sra_dump
{
    my ( $src, $start, $end ) = @_;
    my $tool = File::Spec->catdir( $bin_dir, 'sra-dump' );
    my $dst  = File::Spec->catdir( $out_dir, $^O, $version, $src, 'SRA' );
    mkpath( $dst );
    my $out = File::Spec->catfile( $dst, "$src.txt" );
    my $log = File::Spec->catfile( $dst, 'sra-dump.log' );
    my $cmd = "$tool --start $start --stop $end $src >$out 2>$log";
    print( "\nsra-dump $src : $start - $end\n" );
    return measure_time( $cmd, $log );
}


sub vdb_dump
{
    my ( $src, $start, $end ) = @_;
    my $tool = File::Spec->catdir( $bin_dir, 'vdb-dump' );
    my $dst  = File::Spec->catdir( $out_dir, $^O, $version, $src, 'VDB' );
    mkpath( $dst );
    my $out = File::Spec->catfile( $dst, "$src.txt" );
    my $log = File::Spec->catfile( $dst, 'vdb-dump.log' );
    my $cmd = "$tool $src -R $start-$end >$out 2>$log";
    print( "\nvdb-dump $src : $start - $end\n" );
    return measure_time( $cmd, $log );
}


sub illumina_dump
{
    my ( $src, $start, $end ) = @_;
    my $tool = File::Spec->catdir( $bin_dir, 'illumina-dump' );
    my $dst  = File::Spec->catdir( $out_dir, $^O, $version, $src, 'ILL' );
    mkpath( $dst );
    my $log = File::Spec->catfile( $dst, 'illumina-dump.log' );
    my $cmd = "$tool -N $start -X $end -O $dst $src >$log 2>&1";
    print( "\nillumina-dump $src : $start - $end\n" );
    return measure_time( $cmd, $log );
}


sub fastq_dump
{
    my ( $src, $start, $end ) = @_;
    my $tool = File::Spec->catdir( $bin_dir, 'fastq-dump' );
    my $dst  = File::Spec->catdir( $out_dir, $^O, $version, $src, 'FAQ' );
    mkpath( $dst );
    my $log = File::Spec->catfile( $dst, 'fastq-dump.log' );
    my $cmd = "$tool -N $start -X $end -O $dst $src >$log 2>&1";
    print( "\nfastq-dump $src : $start - $end\n" );
    return measure_time( $cmd, $log );
}


sub perform_test
{
    my ( @cols ) = @_;

    if ( $cols[1] eq 'ABI' )
    {
        abi_dump( $cols[2], $cols[3], $cols[4] );
    }
    elsif ( $cols[1] eq 'SRA' )
    {
        sra_dump( $cols[2], $cols[3], $cols[4] );
    }
    elsif ( $cols[1] eq 'VDB' )
    {
        vdb_dump( $cols[2], $cols[3], $cols[4] );
    }
    elsif ( $cols[1] eq 'ILL' )
    {
        illumina_dump( $cols[2], $cols[3], $cols[4] );
    }
    elsif ( $cols[1] eq 'FAQ' )
    {
        fastq_dump( $cols[2], $cols[3], $cols[4] );
    }
}


sub handle_line
{
    my ( $line ) = @_;
    my @cols = split( / /, $line );
    if ( $cols[0] eq 'S' )
    {
        $bin_dir = $cols[1];
        $version = get_tool_vers( $bin_dir );
        print( "bin: $bin_dir ( version : $version )\n" );
    }
    elsif ( $cols[0] eq 'O' )
    {
        $out_dir = $cols[1];
        print( "out: $out_dir\n" );
    }
    elsif ( $cols[0] eq 'C' )
    {
        $cmp_ver = $cols[1];
        print( "cmp: $cmp_ver\n" );
    }
    elsif ( $cols[0] eq 'T' )
    {
        perform_test( @cols );
    }
}


if ( !defined $schedule_file )
{
    print( "\nYou have to specify an existing schedule-file!\n" );
    print( "\texample: 'dumpers schedule1.txt'\n\n" );
    exit ( -1 );
}

if ( ! -e $schedule_file )
{
    print( "\nThe given test-schedule '$schedule_file' does not exist!\n" );
    exit ( -1 );
}

# loop through the lines of the schedule
my $line;
open ( SCHEDULE, $schedule_file );
while ( ( $line = <SCHEDULE> ), defined( $line ) )
{
    $line =~ s/\r?\n$//;
    if ( $line )
    {
        if ( !( $line =~ /^\#/ ) )
        {
            handle_line( $line );
        }
    }
}

close ( SCHEDULE );