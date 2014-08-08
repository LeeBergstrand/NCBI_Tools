#!/usr/local/bin/perl -w

use strict;

use FindBin qw($Bin);
require "$Bin/common.pl"; # ParseLine
use Getopt::Long "GetOptions";

my %options;
die "GetOptions error" unless (GetOptions(\%options, "force", "help"));
Help() if $options{help};

my %OUT;
my $f = 'SRZ-tiny.sam.*';
open F, $f or die "cannot open $f";
while (<F>) {
    my %fld = ParseLine($_);
    die unless ($fld{RNEXT} =~ /^c/ || $fld{RNEXT} =~ /^\*/);
    Print($fld{RNEXT}, $_);
}
close F;
Close();

sub Print {
    my ($key, $line) = @_;
    my $name = "SRZ-tiny.unaligned.$key";
    unless ($OUT{$key}) {
        if (-e $name && !$options{force}) {
            print "$name exists\n\n";
            Help();
        }
        print "$name\n";
        open(my $fh, ">$name") or die "cannot open $name";
        $OUT{$key} = $fh;
    }
    print { $OUT{$key} } $line or die "cannot print to $name";
}

sub Close {
    foreach (keys %OUT) {
        close $OUT{$_};
    }
}

sub Help {
    print STDERR <<EndText;
Usage:
    $0 [--force] [--help]
EndText

    exit 0;
}
