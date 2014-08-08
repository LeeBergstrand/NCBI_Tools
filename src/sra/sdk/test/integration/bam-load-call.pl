#!/usr/bin/perl -w
use warnings;

# this script has 4 parameters
# (1) ... name of the bam-file to load (WITHOUT THE PATH)
# (2) ... path to the bam-file to load
# (3) ... path to the config-file for the bam-loader
# (4) ... path to be created by the bam-loader

my $argc = @ARGV;

if ( $argc < 4 ) { die "not enough arguments"; }

my $BAM_FILE = $ARGV[0];
my $BAM_PATH = $ARGV[1];
my $CFG_PATH = $ARGV[2];
my $VDB_OUT = $ARGV[3];

print ">>> REMOVING THE VDB-OUTPUT-DIR (in case it exists) <<<\n";
print_and_exec( "rm -rf ./$VDB_OUT" );

print ">>> PERFORMING THE LOAD <<<\n";
print_and_exec( "bam-load -L err -o $VDB_OUT -k $CFG_PATH -Q 0 $BAM_FILE" );
#print_and_exec( "bam-load -L err -o $VDB_OUT -i $BAM_PATH -k $CFG_PATH -Q 0 $BAM_FILE" );

print ">>> $BAM_FILE is loaded into $VDB_OUT <<<\n";

sub print_and_exec
{
    my $t = shift;
    print "$t\n";
    system( $t );
}
