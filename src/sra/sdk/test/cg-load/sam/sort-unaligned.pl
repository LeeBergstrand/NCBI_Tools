#!/usr/local/bin/perl -w
use strict;
use File::Basename "basename";
use Getopt::Long "GetOptions";
my %options;
Help() unless (GetOptions(\%options,
    "in=s", "force", "help", "out=s"));
Help() if ($options{help});
Help() unless ($options{in} && $options{out});
my ($in, $out) = ($options{in}, $options{out});
die "$in not found" unless (-e $in);
die "$in is not a directory" unless (-d $in);
die "i/o directories should be different" if ($in eq $out);
if (-e $out) {
    die "$out is not a directory" unless (-d $out);
    @_ = glob("$out/*");
    if ($#_ > 0) {
        if ($options{force}) {
            `rm -rf $out/*`;
            die "cannot clean $out" if ($?);
        } else {
            die "$out is not empty";
        }
    }
} else {
    mkdir $out or die "cannot mkdir $out";
}
foreach (glob "$in/*") {
    my $input = $_;
    s|^$in/|$out/|;
    my $cmd = "sort -nk8 $input > $_";
    print "$cmd\n";
    `$cmd`;
    die $cmd if ($?);
}
exit 0;
sub Help {
    my $name = basename($0);
    print STDERR <<EndText;
Usage:
    $name --in <input-dir> --out <output-dir> [--force]

    $name -h
    $name --help
                    print this help
EndText

    exit 1;
}
################################################################################
