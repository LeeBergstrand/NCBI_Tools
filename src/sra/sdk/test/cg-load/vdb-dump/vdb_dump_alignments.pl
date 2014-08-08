#!/usr/local/bin/perl -w
################################################################################
# vdb_dump SPOT_ID,SEQ_SPOT_ID,SEQ_READ_ID from PRIMARY/SECONDARY_ALIGNMENT
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
    
    Dump ALIGNMENT tables while converting them into intermediate CG-like format
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options, "bin=s", "help", "sra=s"));
Help(0) if ($options{help});
Help(1) unless ($options{sra});

my $vdb_dump = '/panfs/traces01/trace_software/toolkit/centos64/bin/vdb-dump';
$vdb_dump = "$options{bin}/vdb-dump" if ($options{bin});

my $HEADED;
Dump('PRIMARY');
Dump('SECONDARY');

sub Dump {
    my ($type) = @_;
    my $run = '/panfs/pan1/sra-test/golikov/loader/cg/SRZ-tiny';
    $run = $options{sra};

    my $columns = 'SPOT_ID,SEQ_SPOT_ID,MAPQ,REF_NAME,REF_POS,'
                . 'HAS_REF_OFFSET,REF_OFFSET,REF_ORIENTATION,SEQ_READ_ID';

    my $cmd = "$vdb_dump $run -Nl0 -ftab -C$columns -T${type}_ALIGNMENT";
    print STDERR "$cmd\n";
    open DUMP, "$cmd |" or die "cannot open $cmd";
    while (<DUMP>) {
        chomp;
        my ($SPOT_ID, $SEQ_SPOT_ID, $MAPQ, $REF_NAME, $REF_POS, @refOffset)
            = split /\t/;
        die $_ unless ($#refOffset == 3);
        my $SEQ_READ_ID = $refOffset[3];
        my ($flags_m, $gap1, $gap2, $gap3) = REF_OFFSET2Gaps(@refOffset);
        my $weight = chr($MAPQ + 33);
        unless ($HEADED) {
            print ">TABLE\tSPOT_ID\tSEQ_SPOT_ID\tSEQ_READ_ID\t"
            . "flags\tchromosome\toffsetInChr\tgap1\tgap2\tgap3\tweight\n";
            ++$HEADED;
        }
        print "$type\t$SPOT_ID\t$SEQ_SPOT_ID\t$SEQ_READ_ID\t"
            . "$flags_m\t$REF_NAME\t$REF_POS\t$gap1\t$gap2\t$gap3\t$weight\n"
    }
    close DUMP;
}

sub REF_OFFSET2Gaps {
    my ($HAS_REF_OFFSET, $REF_OFFSET, $REF_ORIENTATION, $SEQ_READ_ID) = @_;
    my $flag_side     = 2;
    my $flag_strand   = 4;
    my $side;
    if ($SEQ_READ_ID == 1)
    {   $side = 0; }
    elsif ($SEQ_READ_ID == 2)
    {   $side = $flag_side; }
    else { die "SEQ_READ_ID = $SEQ_READ_ID" }
    my $strand;
    if ($REF_ORIENTATION eq 'false')
    {   $strand = 0; }
    elsif ($REF_ORIENTATION eq 'true')
    {   $strand = $flag_strand; }
    else { die "REF_ORIENTATION = $REF_ORIENTATION" }
    my $reverted = ($side && !$strand) || (!$side && $strand);
    die "HAS_REF_OFFSET length = " . length($HAS_REF_OFFSET)
        if (length($HAS_REF_OFFSET) != 35);
    my @refOffset = split /, /, $REF_OFFSET;
    my $refIdx = 0;
    my @gap = (0, 0, 0);
    for (my $i = 0; $i < length $HAS_REF_OFFSET; ++$i) {
        my $bit = substr($HAS_REF_OFFSET, $i, 1);
        if      ($bit eq '0') {
        } elsif ($bit eq '1') {
            my $gap;
            if ($reverted) {
                if ($i == 10)
                {   $gap = 1; }
                elsif ($i == 20)
                {   $gap = 2; }
                elsif ($i == 30)
                {   $gap = 3; }
                else
                {   die "HAS_REF_OFFSET = $HAS_REF_OFFSET"; }
            } else {
                if ($i == 5)
                {   $gap = 1; }
                elsif ($i == 15)
                {   $gap = 2; }
                elsif ($i == 25)
                {   $gap = 3; }
                else
                {   die "HAS_REF_OFFSET = $HAS_REF_OFFSET"; }
            }
            die "HAS_REF_OFFSET = $HAS_REF_OFFSET" unless ($gap);
            die "$HAS_REF_OFFSET $REF_OFFSET" if ($refIdx > $#refOffset);
            $gap[--$gap] = $refOffset[$refIdx++];
        } else { die "HAS_REF_OFFSET = $HAS_REF_OFFSET" }
    }
    return ($side | $strand, @gap);
}

################################################################################
# EOF #
