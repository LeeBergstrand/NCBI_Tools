#!/usr/local/bin/perl -w

use strict;
use FindBin qw($Bin);
require "$Bin/common.pl";
use Getopt::Long "GetOptions";

my $LINES = 0;

my %options;
die "GetOptions error"
    unless (GetOptions(\%options, "dir=s", "clean", "force", "help", "keep"));
Help() if ($options{help});
Help() unless ($options{dir} && $#ARGV == 0);
Help() if ($options{clean} && $options{keep});
$options{in} = $ARGV[0];

if (-e $options{dir}) {
    my @f = `/bin/ls $options{dir}`;
    die if ($?);
    if ($#f >= 0) {
        if ($options{clean}) {
            if ($#f >= 0) {
                my $cmd = "/bin/rm $options{dir}/*";
                `$cmd`;
                die if ($?);
            }
        } elsif (!$options{keep}) {
            print "$options{dir} is not empty: clean it or keep the files?\n\n";
            Help();
        }
    }
} else {
    mkdir $options{dir} or die "cannot mkdir $options{dir}";
}

my %OUT;
my $n = 0;
open IN, $options{in} or die "cannot open $options{in}";
while (<IN>) {
    last if ($LINES && ++$n > $LINES);
#   print;
    if (/^@/) {
        Print('header', $_);
    } else {
        my %fld = ParseLine($_);
        die $_ unless ($fld{RNAME});
        Print($fld{RNAME}, $_);
    }
}
close IN;
Close();

sub Print {
    my ($key, $line) = @_;
    my $name = "$options{dir}/$options{in}.$key";
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
    $0 --dir <out-dir> [--clean | --keep] [--force] <in-file>

        Input file: <in-file>
        Output files: <out-dir>/<in-file>.suffix

    $0 -h
    $0 --help
        print this help
EndText

    exit 0;
}
