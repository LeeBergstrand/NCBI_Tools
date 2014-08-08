#!/usr/bin/perl -w

#----------------------------------------------------------
# this script compares the bam-file versus the vdb-database
#----------------------------------------------------------

if ( @ARGV == 0 )
{
    print( "\n-------------------------------------------------------------------------\n" );
    print( "USAGE: validvdb.pl bam-file vdb-db outdir\n" );
    print( "bam-file ... the full path to the source-bam-file\n" );
    print( "vdb-db   ... the full path of a vdb-directory containing a loaded bam-file\n" );
    print( "outdir   ... opt. output-dir, if not present a pid-based one is used\n" );
    print( "-------------------------------------------------------------------------\n\n" );
}
else
{
    my $bamsrc = $ARGV[ 0 ];
    my $vdb    = $ARGV[ 1 ];
    my $outdir = $ARGV[ 2 ];

    if ( ! -e $bamsrc )
    {
        die "$bamsrc does not exist!\n"
    } 
    if ( ! -d $vdb )
    {
        die "$vdb does not exist!\n"
    } 

    if ( defined $outdir )
    {
        if ( $outdir eq "" )
        {
            $outdir = "tmp_$$";
        }
    }
    else
    {
        $outdir = "tmp_$$";
    }

    print ( ">>> VALIDVDB <<<\n" );
    print ( "src : $bamsrc\n" );
    print ( "vdb : $vdb\n" );
    print ( "out : $outdir\n" );

    if ( ! -d $outdir )
    {
        mkdir $outdir or die $!;
    }

    #extract data from the bam-file:
    print ( ">>> SAM-TOOLS-DUMP <<<\n" );
    system( "sam-tools-dump.pl $bamsrc $outdir" );
    if ( $? == 0 )
    {
        #extract data from the vdb-database:
        system( "vdb-tools-dump.pl $vdb $outdir" );
    }
}
