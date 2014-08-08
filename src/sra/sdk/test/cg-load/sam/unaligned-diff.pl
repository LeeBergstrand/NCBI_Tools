#!/usr/local/bin/perl -w

use strict;

use Getopt::Long "GetOptions";

my %options;
die "GetOptions error" unless (GetOptions(\%options, "help"));
Help() if ($options{help});
Diff('100.sam.chrM.filtered.nomatch.sorted',
    'SRZ-tiny.unaligned.chrM.sorted');
sub Diff {}
sub Help {
    print STDERR <<EndText;
Compares all pairs of:
( SRZ-tiny.unaligned.XXX.sorted 100.sam.XXX.filtered.nomatch.sorted )
found in the current directory
EndText

    exit 0;
}

=begin COMMENT

use FindBin qw($Bin);
require "$Bin/common.pl";
use File::Basename "basename";

Help() if ($#ARGV < 1 || $#ARGV > 2);

my $LINES = 0;
my ($N1, $N2);
if ($#ARGV == 1) {
    Diff($ARGV[0], $ARGV[1]);
} else {
    my $dir = $ARGV[0];
    Help("Bad directory $dir\n\n") unless (-d $dir);
    opendir(DIR, $dir) or die "cannot opendir $dir";
    my ($pfx1, $pfx2) = @ARGV[1..2];
    my (%file1, %file2);
    while ($_ = readdir(DIR)) {
        next unless (/\.filtered$/);
        if (/^$pfx1(.+)\.filtered$/) {
            die $_ if ($file1{$1});
            $file1{$1} = $_;
        } elsif (/^$pfx2(.+)\.filtered$/) {
            die $_ if ($file2{$1});
            $file2{$1} = $_;
        } else {
            die $_;
        }
    }
    closedir DIR;
    die if (keys %file1 != keys %file2);
    foreach my $sfx (keys %file1) {
        die $file1{$sfx} unless ($file2{$sfx});
        Diff("$dir/$file1{$sfx}", "$dir/$file2{$sfx}");
    }
}

sub Diff {
    my ($file1, $file2) = @_;
    print "$file1 $file2\n";
    open FILE1, $file1 or die "cannot open $file1";
    open FILE2, $file2 or die "cannot open $file2";
    my $n = 0;
    ($N1, $N2) = (0, 0);
    foreach my $line1 (<FILE1>) {
        ++$N1;
        my $line2;
        next if ($line1 =~ /^@/);
        last if ($LINES && ++$n > $LINES);
        while (1) {
            $line2 = <FILE2>;
            ++$N2;
            last unless ($line2 =~ /^@/);
        }
        DiffLine($line1, $line2);
    }
}

sub DiffLine {
    my ($line1, $line2) = @_;
    my %fld1 = ParseLine($line1);
    my %fld2 = ParseLine($line2);
    my @fields
 = qw(QNAME FLAG RNAME POS MAPQ CIGAR RNEXT PNEXT TLEN SEQ QUAL RG GS GC GQ NM);
    @fields = qw(RNAME POS MAPQ       RNEXT PNEXT      SEQ QUAL); # TLEN CIGAR
    @fields = qw(RNAME POS MAPQ                        SEQ QUAL);
    foreach (@fields) {
        if (DiffFld($fld1{$_}, $fld2{$_}, $_, $line1, $line2)) {
            $line1 = $line2 = '';
        }
    }
    if ($line1 && $line2) {
        print "+ $line1+ $line2\n" if (0);
    } else {
        print "\n";
    }
}

sub DiffFld {
    my ($fld1, $fld2, $key, $line1, $line2) = @_;
    my $diff;
    if (! defined $fld1 && ! defined $fld2) {
        PrintDiff("> $key", $line1, $line2);
        ++$diff;
    } elsif (! defined $fld1) {
        PrintDiff("> $key : $fld2", $line1, $line2);
        ++$diff;
    } elsif (! defined $fld2) {
        PrintDiff("> $key : $fld1", $line1, $line2);
        ++$diff;
    } elsif ($fld1 ne $fld2) {
        PrintDiff("- $key : $fld1\t$fld2", $line1, $line2);
        ++$diff;
    } else {
#       PrintDiff "+ $key : $fld1\t$fld2", $line1, $line2;
    }
    return $diff;
}

sub PrintDiff {
    my ($diff, $line1, $line2) = @_;
    print "$N1 $line1" if ($line1);
    print "$N2 $line2" if ($line2);
#   print "$diff\n";
}

=cut
