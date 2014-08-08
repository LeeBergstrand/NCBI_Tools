#!/usr/local/bin/perl -w
################################################################################
# vdb_dump SEQUENCE SPOT_ID etc[, mapping side, PRIMARY_ALIGNMENT_ID]
################################################################################

use strict;
use File::Basename "basename";
use Getopt::Long "GetOptions";

sub Help {
    my ($exit) = @_;
    $exit = 0 unless (defined $exit);
    my $name = basename($0);
    print <<EndText;
Usage:
    $name --sra <run path> --bin <vdb-dump binary directory>
    
    Dump SEQUENCE table while converting it into intermediate CG-like format
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options, "bin=s", "help", "sra=s"));
Help(0) if ($options{help});
Help(1) unless ($options{sra});

$options{bin} = '/home/klymenka/OUTDIR/bin64' unless ($options{bin});

my $LeftHalfDnbNoMatches    = 1;
my $RightHalfDnbNoMatches   = 4;

my $vdb_dump = '/panfs/traces01/trace_software/toolkit/centos64/bin/vdb-dump';
$vdb_dump = "$options{bin}/vdb-dump";
my $columns = 'SPOT_ID,PRIMARY_ALIGNMENT_ID,READ,QUALITY';
my $run = '/panfs/pan1/sra-test/golikov/loader/cg/SRZ-tiny';
$run = $options{sra};
my $cmd = "$vdb_dump $run -Nl0 -ftab -C$columns";
print STDERR "$cmd\n";
my $HEADED;
open DUMP, "$cmd |" or die "cannot open $cmd";
while (<DUMP>) {
    chomp;
    my ($SPOT_ID, $PRIMARY_ALIGNMENT_ID, $READ, $QUALITY) = split /\t/;
    my @primaryAlignmentId = split /, /, $PRIMARY_ALIGNMENT_ID;
    die $_ unless ($#primaryAlignmentId == 1);
    my $scores = '';
    my @QUALITY = split /, /, $QUALITY;
    die $_ unless ($#QUALITY == 69);
    $scores .= chr($_ + 33) foreach (@QUALITY);
    my $flags = 0;
    if ($primaryAlignmentId[0] == 0)
    {   $flags |= $LeftHalfDnbNoMatches }
    if ($primaryAlignmentId[1] == 0)
    {   $flags |= $RightHalfDnbNoMatches }
    my $printed = 0;
    unless ($HEADED) {
        print ">SPOT_ID\tflags\tREAD\tscores\tSide\tPRIMARY_ALIGNMENT_ID\n";
        ++$HEADED;
    }
    for (my $i = 0; $i < 2; ++$i) {
        if ($primaryAlignmentId[$i]) {
            print "$SPOT_ID\t$flags\t$READ\t$scores\t";
            print $i ? "R" : "L";
            print "\t$primaryAlignmentId[$i]";
            print "\n";
            $printed = 1;
        }
    }
    print "$SPOT_ID\t$flags\t$READ\t$scores\n" unless ($printed);
}
close DUMP;

################################################################################
# EOF #
