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
use File::Path;
use IO::File;

#----------------------------------------------------------
# this script produces the dump-output from vdb
#----------------------------------------------------------
my $tmpfs;
my $tmpdir;
my $log;
my $debug = 0;

sub log_it
{
    my $now = localtime();
    my $fmt = shift;
    printf ("%s: $fmt",$now,@_) if ($debug);
    printf{$log} ("%s: $fmt",$now,@_);
}

# use /export/home/TMP if it exists or /tmp if it does not
$tmpfs = "/export/home/TMP" if (-d "/export/home/TMP");
$tmpfs = "/tmp" unless $tmpfs;

# create a directory in the temp fs for this run using the PID to claim it as ours
$tmpdir = "$tmpfs/vdb-tools-dump.$$";

my $nargs = scalar @ARGV;

if ( $nargs == 0  )
{
    print( "\n-------------------------------------------------------------------------\n" );
    print( "USAGE: vdb-tools-dump.pl vdb-db outdir [-cs]\n" );
    print( "vdb-db   ... the full path of a vdb-directory containing a loaded bam-file\n" );
    print( "outdir   ... output-dir\n" );
    print( "-cs      ... optional switch to replace READ with CSREAD\n" );
    print( "-------------------------------------------------------------------------\n\n" );
}
else
{
    my $vdbsrc = $ARGV[ 0 ];
    my $outdir = $ARGV[ 1 ];
    my $cs = 0;

    if ( $nargs > 2 )
    {
        if ( $ARGV[ 2 ] eq "-cs" ) { $cs = 1; }
    }

    if (( ! -d $vdbsrc ) && ( ! -f $vdbsrc ))
    {
        die "$vdbsrc does not exist!\n"
    }
    if ( ! -d $outdir )
    {
        die "$outdir does not exist!\n"
    }

    open ($log, ">", "$outdir/vdb-tools-dump.log") or die "Cannot open log file";
    log_it ("vdb-tools-dump started for $vdbsrc to $outdir/vdbtools\n");

    my $tooldir = "$tmpdir/vdbtools";
    if ( ! -d $tooldir )
    {
        mkpath $tooldir or die $!;
    }

    my $al_dir = "$tooldir/aligned";
    if ( ! -d $al_dir )
    {
        mkpath $al_dir or die $!;
    }

    my $ual_dir = "$tooldir/unaligned";
    if ( ! -d $ual_dir )
    {
        mkpath $ual_dir or die $!;
    }

    my $otooldir = "$outdir/vdbtools";
    mkpath $otooldir or die $! unless (-d $otooldir);

    my $oal_dir = "$otooldir/aligned";
    mkpath $oal_dir or die $! unless (-d $oal_dir);

    my $oual_dir = "$otooldir/unaligned";
    mkpath $oual_dir or die $! unless (-d $oual_dir);

    my $rowcount = 0;
    my %tables = ();
    detect_tables( \$vdbsrc, \%tables );

    my %refnames = ();
    ####################################################################
    #   PREPARE vdb-dump command for PRIMARY_ALIGNMENT ( aligned rows )
    ####################################################################
    #my $cols = "READ,CIGAR_LONG,CIGAR_SHORT,MAPQ,SPOT_GROUP,REF_NAME";
    my $cols = "READ,";
    # if ( $cs > 0 )
    # { $cols = "CSREAD," };
    # else
    # { $cols = "READ," };
    $cols .= "MAPQ,REF_NAME,REF_POS,MATE_REF_NAME,MATE_REF_POS,";
    $cols .= "'(INSDC:quality:text:phred_33)QUALITY',";
    $cols .= "REF_ORIENTATION,TEMPLATE_LEN";
    my $table = "PRIMARY_ALIGNMENT";
    dump_aligned( \$table, \%tables, \$vdbsrc, \$cols, \$al_dir, \%refnames, \$rowcount );
    log_it("$rowcount rows dumped\n");

    ####################################################################
    #   PREPARE vdb-dump command for SECONDARY_ALIGNMENT( aligned rows )
    ####################################################################
    $table = "SECONDARY_ALIGNMENT";
    $rowcount = 0;
    dump_aligned( \$table, \%tables, \$vdbsrc, \$cols, \$al_dir, \%refnames, \$rowcount );
    log_it( "$rowcount rows dumped\n" );

    close_sort_refnames( \%refnames, \$outdir );

    ####################################################################
    #   PREPARE vdb-dump command for SEQUENCE ( unaligned rows )
    ####################################################################
    $table = "SEQUENCE";
    if ( exists( $tables{ $table } ) )
    {
        $cols = "PRIMARY_ALIGNMENT_ID,";
        if ( $cs > 0 )
            { $cols .= "CSREAD,"; }
        else
            { $cols .= "READ,"; }
        $cols .= "'(INSDC:quality:text:phred_33)QUALITY',";
        $cols .= "SPOT_GROUP,READ_START,READ_LEN,READ_TYPE";
        my $vdb = "vdb-dump $vdbsrc -T $table -f tab -C $cols";
        my $vdb_row;

        $rowcount = 0;
        log_it("dumping >$table<\n");
        my %spotgroups = ();
        open ( DUMP, "-|", "$vdb" ) or die "vdb-dump ($table) failed";
        while ( ( $vdb_row = <DUMP> ), defined( $vdb_row ) )
        {
            chomp( $vdb_row );
            unaligned_row( \$vdb_row, \$ual_dir, \%spotgroups, \$cs );
            $rowcount++;
        }
        close ( DUMP );
        log_it( "$rowcount rows dumped\n" );
        close_sort_spotgroups( \%spotgroups, \$outdir );
    }
    else
    {
        log_it( "table: $table is missing\n" );
    }


    system ("rm -fr $tmpdir");
    exit (0);
}


####################################################################
#   dump either PRIMARY_ALIGNMENT or SECONDARY_ALIGNMENT-table
####################################################################
sub dump_aligned
{
    my( $table_ref, $tables_ref, $vdbsrc_ref, $cols_ref,
        $dir_ref, $refnames_ref, $rowcount_ref ) = @_;

    if ( exists( $$tables_ref{ $$table_ref } ) )
    {
        log_it( "dumping >$$table_ref<\n" );
        my $vdb = "vdb-dump $$vdbsrc_ref -T $$table_ref -f tab -C $$cols_ref";
        my $vdb_row;
        ####################################################################
        #   pipe the joined aligned vdb-dump-output in the splitter
        #   we split on REFNAME ( not there yet )
        ####################################################################

        open ( DUMP, "-|", "$vdb" ) or die "vdb-dump ($$table_ref) failed";
        while ( ( $vdb_row = <DUMP> ), defined( $vdb_row ) )
        {
            chomp( $vdb_row );
            aligned_row( \$vdb_row, $dir_ref, $refnames_ref );
            $$rowcount_ref++;
        }
        close ( DUMP );
    }
    else
    {
        log_it( "table: >$$table_ref< is missing\n" );
    }
}


####################################################################
#   split the row into files by REFNAME + quantiziced POS...
####################################################################
sub aligned_row
{
    my ( $row_ref, $dir_ref, $hash_ref ) = @_;
    my @cols = split( /\t/, $$row_ref );
    my $read = $cols[ 0 ];
    my $mapq = $cols[ 1 ];
    my $rname = $cols[ 2 ];
    my $pos = $cols[ 3 ];
    my $rnext = $cols[ 4 ];
    my $pnext = $cols[ 5 ];
    my $qual = $cols[ 6 ];
    my $orient = $cols[ 7 ];
    my $tlen = $cols[ 8 ];
    my $r_orient = 0;
    my $mr_orient = 0;

    if ( $rnext eq $rname ) { $rnext = "=";  }
    $pos++;
    if ( $pnext ne "" )
        {   if ( $pnext > 0 ) { $pnext++; } }
    else
        {   $pnext = '0';   }

#    if ( $orient eq 'true' )  { $qual = reverse( $qual ); $r_orient = 1; }

    my $pos_bin = int( $pos / 10000000 );
    my $f_core_name = "$$dir_ref/core_$rname" . "_" . $pos_bin;
    my $f_ext_name = "$$dir_ref/ext_$rname" . "_" . $pos_bin;
    if ( !exists( $$hash_ref{ $f_core_name } ) )
        { $$hash_ref{ $f_core_name } = new IO::File( $f_core_name, "w" ) or die $!; }
    if ( !exists( $$hash_ref{ $f_ext_name } ) )
        { $$hash_ref{ $f_ext_name } = new IO::File( $f_ext_name, "w" ) or die $!; }

    if ( defined( $read ) )
    {
        $$hash_ref{ $f_core_name }->print( "$pos\t$read\t$qual\t$mapq\n" );
        $$hash_ref{ $f_ext_name }->print( "$pos\t$tlen\t$rnext\t$pnext\n" );
    }
}


####################################################################
#   close all refnames-filehandles, and sort them
####################################################################
sub close_sort_refnames
{
    my ( $hash_ref,$outdir_ref ) = @_;
    my ( $rn_fname, $rn_fhandle );
    my $cnt = 0;
    while ( ( $rn_fname, $rn_fhandle ) = each( %$hash_ref ) )
    {
        $rn_fhandle->close();
        my $outfile = substr ($rn_fname,1+length($tmpdir));
        if (substr($outfile,0,4) eq "ext_")
        {
            system("sort","-t\t","-n","-k1","-k2",$rn_fname,"--output=$$outdir_ref/$outfile" );
        }
        else
        {
            system( "sort $rn_fname -o $$outdir_ref/$outfile" );
        }
        $cnt++;
    }
    log_it( "$cnt aligned files sorted\n" );
}


####################################################################
#   reverse and complement the base-string
####################################################################
sub reverse_read
{
    my ( $src_ref ) = @_;
    my $rev = "";
    my $count = length( $$src_ref );
    my $i;
    for ( $i = $count; $i >= 0; $i-- )
    {
        my $base1 = substr( $$src_ref, $i, 1 );
        if ( $base1 eq "A" )
            { $rev .= "T"; }
        elsif ( $base1 eq "C" )
            { $rev .= "G"; }
        elsif ( $base1 eq "G" )
            { $rev .= "C"; }
        elsif ( $base1 eq "T" )
            { $rev .= "A"; }
        else
            { $rev .= $base1; }
    }
    $$src_ref = $rev;
}


####################################################################
#   split the row into files by SPOTGROUP...
####################################################################
sub unaligned_row
{
    my ( $row_ref, $dir_ref, $hash_ref, $cs_ref ) = @_;
    my @cols = split( /\t/, $$row_ref );
    my @alignment_ids = split( /,/, $cols[ 0 ] );
    my @rd_start = split( /,/, $cols[ 4 ] );
    my @rd_len = split( /,/, $cols[ 5 ] );
    my @rd_type = split( /,/, $cols[ 6 ] );

    my $idx = 0;
    my ( $al_id, $start, $len, $read, $qual_r, $fname );
    foreach( @alignment_ids )
    {
        # remove whitespace and quotes from the alignment-id
        chomp( $al_id = $_ );
        $al_id =~ s|"||g;

        if ( $al_id == 0 )
        {
            chomp( $start = $rd_start[ $idx ] );
            chomp( $len = $rd_len[ $idx ] );
            $start =~ s|"||g;
            $len =~ s|"||g;

            if ( $len > 0 )
            {
                $read = substr( $cols[ 1 ], $start, $len );
                $qual_r = substr( $cols[ 2 ], $start, $len );

#                if ( index( $rd_type[ $idx ], "SRA_READ_TYPE_REVERSE" ) >= 0 )
#                {
#                    if ( $$cs_ref > 0 )
#                        { $read = reverse( $read ); }
#                    else
#                        { reverse_read( \$read ); }
#                    $qual_r = reverse( $qual_r );
#                }

                $fname = "$$dir_ref/$cols[3]";
                if ( !exists( $$hash_ref{ $fname } ) )
                {
                    $$hash_ref{ $fname } = new IO::File( $fname, "w" ) or die $!;
                }
                $$hash_ref{ $fname }->print( "$read\t$qual_r\n" );
            }
        }
        $idx++;
    }
}


####################################################################
#   close all spotgroups-filehandles, and sort them
####################################################################
sub close_sort_spotgroups
{
    my ( $hash_ref,$outdir_ref ) = @_;
    my ( $sg_fname, $sg_fhandle );
    my $cnt = 0;
    while ( ( $sg_fname, $sg_fhandle ) = each( %$hash_ref ) )
    {
        $sg_fhandle->close();
        log_it( "sorting: $sg_fname\n" );
        system( "sort $sg_fname -o $$outdir_ref/$sg_fname" );
        $cnt++;
    }
    log_it( "$cnt unaligned files sorted\n" );
}


####################################################################
#   detect which tables do exist in vdbsrc
####################################################################
sub detect_tables
{
    my ( $dir_ref, $hash_ref ) = @_;
    my ( $enum_row );
    open ( ENUM, "-|", "vdb-dump $$dir_ref -E" ) or die "vdb-dump (enum tables) failed";
    while ( ( $enum_row = <ENUM> ), defined( $enum_row ) )
    {
        chomp( $enum_row );
        my @cols = split( / /, $enum_row );
        if ( $cols[ 0 ] eq "tbl" )
        {
            $$hash_ref{ $cols[2] } = $cols[2];
        }
    }
    close ( ENUM );
}
