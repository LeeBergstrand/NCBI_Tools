#!/usr/bin/perl -w
use warnings;

my $ACC = "NC_000001";
my $DST = "newtab";
my $SHOW_COLUMNS = "SEQ_ID,DEF_LINE,MAX_SEQ_LEN,TOTAL_SEQ_LEN";

print ">>> REMOVING THE DESTIONATION <<<\n";
print_and_exec( "rm -rf newtab" );

print ">>> PERFORMING THE REFERENCE-SEQUENCE-LOAD <<<\n";
print_and_exec( "refseq-load -f $ACC -d $DST -cno" );

print "\n>>> SHOW STATIC COLUMNS IN THE CREATED TABLE <<<\n";
print_and_exec( "vdb-dump $DST -a -R1 -C $SHOW_COLUMNS" );

print "\n>>> DUMP THE READ-COLUMN FROM THE CREATED TABLE <<<\n";
print_and_exec( "vdb-dump $DST -a -N -l0 -CREAD > refseq.dump" );

print "\n>>> REMOVE THE NEWLINE FROM THE DUMPED FILE <<<\n";
print_and_exec( "cat refseq.dump | tr -d '\\n' > refseq1.dump" );

print "\n>>> FETCH THE SAME ACCESSION VIA ID-FETCH FOR COMPARISON <<<\n";
print_and_exec( "idfetch -s \"ref|$ACC\" -t 5 > sequence.id" );

print "\n>>> REMOVE THE FIRST LINE FROM THE FETCHED DATA <<<\n";
remove_first_line( "sequence.id", "s1.id" );

print "\n>>> REMOVE THE NEWLINE FROM THE FETCHED DATA <<<\n";
print_and_exec( "cat s1.id | tr -d '\\n' > s2.id" );

print "\n>>> COMPARE THE DUMPED READ-COLUMN WITH THE FETCHED DATA <<<\n";
print_and_exec( "diff -s --brief refseq1.dump s2.id" );

print "\n>>> ERASE TEMPORARY FILES <<<\n";
print_and_exec( "rm -f s1.id s2.id refseq.dump refseq1.dump" );

sub print_and_exec
{
    my $t = shift;
    print "$t\n";
    system( $t );
}

sub remove_first_line
{
    my $infile = shift;
    my $outfile = shift;
    open my $in, '<', $infile;
    open my $out, '>', $outfile;
    <$in>; #discards first line
    print $out $_ while ( <$in> );
    close $out;
    close $in;
}
