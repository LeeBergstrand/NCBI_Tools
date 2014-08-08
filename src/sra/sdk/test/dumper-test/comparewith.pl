#! /usr/bin/perl -w
use File::Path;
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


sub only_filenames
{
    my ( $listref ) = @_;
    foreach my $item ( @$listref )
    {
        my ( $filename, $filepath, $fileext ) = fileparse( $item );
        $item = $filename;
    }
}


sub cmp_dir
{
    my $res = 0;
    my ( $listref, $d1_ref, $d2_ref ) = @_;
    foreach my $item ( @$listref )
    {
        if ( $item ne "." && $item ne ".." )
        {
            my $f1 = File::Spec->catdir( $$d1_ref, $item );
            my $f2 = File::Spec->catdir( $$d2_ref, $item );
            if ( ! -e $f1 )
            {
                $res++;
                print( "\t$f1 not found!\n" );
            }
            if ( ! -e $f2 )
            {
                $res++;
                print( "\t$f2 not found!\n" );
            }
        }
    }
    return $res;
}

sub dir_cmp
{
    my $res = 0;
    my ( $subname, $dir1, $dir2 ) = @_;

    my $d1 = File::Spec->catdir( $dir1, $subname );
    my $d2 = File::Spec->catdir( $dir2, $subname );
    print( "\ncomparing $subname:\n\t$d1\n\t$d2\n" );

    if ( ! -d $d1 )
    {
        print( "\tdirectory does not exist: '$d1'\n" );
        return( -1 );
    }

    if ( ! -d $d2 )
    {
        print( "\tdirectory does not exist: '$d2'\n" );
        return( -1 );
    }

    opendir( DIR1h, $d1 );
    opendir( DIR2h, $d2 );

    my @f1 = readdir( DIR1h );
    only_filenames( \@f1 );

    my @f2 = readdir( DIR2h );
    only_filenames( \@f2 );

    $res += cmp_dir( \@f1, \$d1, \$d2 );
    $res += cmp_dir( \@f2, \$d1, \$d2 );

    if ( $res == 0 )
    {
        foreach my $item ( @f1 )
        {
            if ( $item ne "." && $item ne ".." )
            {
                my $file1 = File::Spec->catdir( $d1, $item );
                my $file2 = File::Spec->catdir( $d2, $item );
                $res += system ( "diff $file1 $file2 -q" );
            }
        }
    }


    if ( $res == 0 )
    {
        print( "\tresult of comparison: OK\n" );
    }

    return $res;
}


my $outdir = $ARGV[ 0 ];
my $cmpver = $ARGV[ 1 ];
my $toolpath = $ARGV[ 2 ];

if ( !defined $toolpath )
{
    $toolpath = "";
}

if ( !defined $outdir )
{
    print( "\nYou have to specify an existing directory!\n" );
    print( "\texample: 'comparewith /panfs/pan1/sra-test/username/dumpout 1.1.6'\n\n" );
    exit ( -1 );
}

if ( !defined $cmpver )
{
    print( "\nYou have to specify a version to compare with!\n" );
    print( "\texample: 'comparewith /panfs/pan1/sra-test/username/dumpout 1.1.6'\n\n" );
    exit ( -1 );
}

if ( ! -d $outdir )
{
    print( "\nThe output directory '$outdir' does not exist!\n" );
    exit ( -1 );
}

#let the File::Spec 'class' select the right path separator for the current OS!
@dirs = ( $outdir, get_tool_vers( $toolpath ), $^O );
my $current_out = File::Spec->catdir( @dirs );

@dirs = ( $outdir, $cmpver, $^O );
my $cmp_dir = File::Spec->catdir( @dirs );

print( "\ncomparing:\n" );
print( "\t$current_out\n" );
print( "\t$cmp_dir\n\n" );

if ( ! -d $current_out )
{
    print( "\nThe directory '$current_out' does not exist!\n" );
    exit ( -1 );
}
    
if ( ! -d $cmp_dir )
{
    print( "\nThe directory '$cmp_dir' does not exist!\n" );
    @dirs = ( $outdir, $cmpver, "linux" );
    $cmp_dir = File::Spec->catdir( @dirs );
    print( "...trying '$cmp_dir' instead!\n" );

    if ( ! -d $cmp_dir )
    {
        print( "\nThe directory '$cmp_dir' does not exist!\n" );
        exit ( -1 );
    }
}

my $err = 0;
my $ok = 0;

#perform the abi-compare...
if ( dir_cmp( "abi-dump", $current_out, $cmp_dir ) == 0 ) { ++$ok; } else { ++$err; }

#perform the sra-compare...
if ( dir_cmp( "sra-dump", $current_out, $cmp_dir ) == 0 ) { ++$ok; } else { ++$err; }

#perform the vdb-compare...
if ( dir_cmp( "vdb-dump", $current_out, $cmp_dir ) == 0 ) { ++$ok; } else { ++$err; }

#perform the illumina-compare...
if ( dir_cmp( "illumina-dump", $current_out, $cmp_dir ) == 0 ) { ++$ok; } else { ++$err; }

#perform the fastq-compare...
if ( dir_cmp( "fastq-dump", $current_out, $cmp_dir ) == 0 ) { ++$ok; } else { ++$err; }

print( "\n$ok comparisons successful / $err comparisons failed\n" );
