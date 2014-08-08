#! /usr/bin/perl -w
use File::Basename;

my $to_sort = $ARGV[ 0 ];

my @extfiles     = glob( "$to_sort/samtools/aligned/ext_*" );
my @moreextfiles = glob( "$to_sort/vdbtools/aligned/ext_*" );

push ( @extfiles, @moreextfiles );

foreach $file ( @extfiles )
{
    my $basename;
    my $path;
    my $suffix;

    ( $basename, $path, $suffix ) = fileparse ( $file );
    my $unsfile = $path . "uns-"  .$basename . $suffix;
    print( "$unsfile => $file\n" );
    rename( $file, $unsfile );
    system( "sort", "-t\t", "-n", "-k1", "-k2", $unsfile, "--output=$file" );
    system( "rm", $unsfile );
}


__END__
