#!/usr/local/bin/perl -w
use strict;
use FindBin qw($Bin);
require "$Bin/common.pl";
my @list = @ARGV;
@list = glob('*h') if ($#ARGV < 0);
foreach (@list) {
    nomatch_check($_);
}
sub nomatch_check {
    my ($f) = @_;
    print "$f\n";
    open F, $f or die "cannot open $f";
    my $l = 1;
    while (<F>) {
        my %fld = ParseLine($_);
        die "$fld{MAPQ}\n" unless ($fld{MAPQ} == 0);
        die "$fld{CIGAR}\n" unless ($fld{CIGAR} eq '*');
        die "RNAME $fld{RNAME} $fld{RNEXT} $l $_\n"
            unless ($fld{RNAME} eq $fld{RNEXT} || $fld{RNEXT} eq '=');
        die "$fld{POS} $fld{PNEXT}\n" unless ($fld{POS} == $fld{PNEXT});
        die "$fld{TLEN}\n" unless ($fld{TLEN} == 0);
        ++$l;
    }
    close F;
}
################################################################################
