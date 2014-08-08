#!/usr/local/bin/perl -w

use strict;
use File::Basename "basename";
use Getopt::Long "GetOptions";

my %options;
die "GetOptions error" unless (GetOptions(\%options,
    "clean", "input=s", "force", "help", "keep", "output=s"));
Help() if ($options{help});
Help() unless ($options{input} && $options{output});

unless (-e $options{input}) {
    Help("$options{input} not found\n\n");
}
if (-e $options{output}) {
    my @f = `/bin/ls $options{output}`;
    die if ($?);
    if ($#f >= 0) {
        if ($options{clean}) {
            if ($#f >= 0) {
                my $cmd = "/bin/rm $options{output}/*";
                `$cmd`;
                die $cmd if ($?);
            }
        } elsif (!$options{keep}) {
            Help("$options{output} is not empty: " .
                "clean it or keep the files?\n\n");
        }
    }
} else {
    mkdir $options{output} or die "cannot mkdir $options{output}";
}

my %OUT;
opendir(IN, $options{input}) or die "cannot opendir $options{input}";
while ($_ = readdir(IN)) {
    next if (/^\.{1,2}$/);
    my $out = "$options{output}/$_";
    if (-e $out && !$options{force}) {
        Help("$out exists\n\n");
    }
    s/\*/\\\*/;
    my $cmd = "/bin/sort";
    #           4: POS, 5: MAPQ# 10: SEQ 
    $cmd .= " -n -k4,4 -k5,5" unless (/\.header$/); # -k10,10
    $cmd .= " $options{input}/$_ > $out";
    print "$cmd\n";
    `$cmd`;
    die $cmd if ($?);
}
closedir(IN);

sub Help {
    my $name = basename($0);
    print STDERR @_ if (@_);
    print STDERR <<EndText;
Usage:
    $name --input <in-dir> --output <out-dir> [--clean | --keep] [--force]

    $name -h
    $name --help
                    print this help
EndText

    exit 0;
}

################################################################################
