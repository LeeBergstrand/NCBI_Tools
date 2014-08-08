#!/usr/local/bin/perl -w
use strict;
use FindBin qw($Bin);
require "$Bin/common.pl"; # ParseLine
use Cwd "getcwd";
use File::Basename "basename";
use Getopt::Long "GetOptions";
my %options;
Help() unless
    (GetOptions(\%options, "cg=s", "in=s", "force", "help", "ncbi=s", "out=s"));
Help() if ($options{help});
Help()
    unless ($options{cg} && $options{out} && $options{ncbi} && $options{out});
Help("$options{in} is not a directory\n\n") unless (-d $options{in});
my $cwd = getcwd();
chdir $options{in} or die "cannot cd $options{in}";
my @in = glob("*");
Help("$options{in} is empty\n\n") unless ($#in > 0);
chdir $cwd or die "cannot cd $cwd";
my %out;
( $out{aligned}           , $out{unaligned}        , $out{header},
  $out{star}) =
 ("$options{out}.aligned","$options{out}.unaligned","$options{out}.header",
  "$options{out}.aligned-unmapped");
$options{unaligned} = $out{unaligned};
$options{star} = $out{star};
foreach (keys %out) {
    my $out = $out{$_};
    if (-e $out) {
        Help("$out is not a directory\n\n") unless (-d $out);
        my @out = glob("$out/*");
        if ($#out > 0) {
            Help("$out is not empty\n\n") unless ($options{force});
            `rm $out/*`;
            die if ($?);
        }
    } else {
        mkdir $out or die "cannot mkdir $out";
    }
}
my %OUT;
foreach (@in) {
    my $cg;
    print "$_\t";
    if (/^$options{cg}/) {
        ++$cg;
    } else {
        die "unrecognized file" unless (/^$options{ncbi}/);
    }
    my $out;
    if (/\.filtered$/) {
        $out = $out{aligned};
    } elsif (/\.nomatch$/) {
        if ($cg) {
            $out = $out{unaligned};
        } else {
            die "nomatch ncbi file";
        }
    } elsif (/\.header$/) {
        $out = $out{header};
    } elsif (/\.STAR$/) {
        if ($cg) {
            $out = $out{star};
        } else {
            SplitNcbiUnaligned($_, $`);
        }
    } else {
        die "unknown file";
    }
    if ($out) {
        print $out;
        my $cmd = "cp -p $options{in}/$_ $out";
        `$cmd`;
        die "cannot $cmd" if ($?);
    }
    print "\n";
}
sub SplitNcbiUnaligned {
    my ($in, $out) = @_;
    $in = "$options{in}/$in";
    open F, $in or die "cannot open $in";
    while (<F>) {
        my %fld = ParseLine($_);
        die unless ($fld{RNEXT} =~ /^c/ || $fld{RNEXT} =~ /^\*/);
        Print($out, $fld{RNEXT}, $_);
    }
    close F;
    Close();
}
sub Print {
    my ($in, $key, $line) = @_;
    my ($dir, $sfx);
    if ($key ne '*') {
        ($dir, $sfx) = ($options{unaligned}, $key);
    } else {
        ($dir, $sfx) = ($options{star}, 'STAR');
    }
    my $name = "$dir/$in.$sfx";
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
    my $name = basename($0);
    print STDERR @_ if (@_);
    print STDERR <<EndText;

Usage: $name --in <input-dir> --out <output-dir-prefix> [--force]
    --cg <cg-files-prefix> --ncbi <ncbi-files-prefix>

    --force : clean existing output directory

Unaligned NCBI fife will be split by RNEXT.

Example:

$name --in filtered --out separated --cg 100 --ncbi SRZ-tiny

Read files in "filtered" directory.
"100*" files are CG files;
"SRZ-tiny*" files are NCBI files.

Write:
    aligned   to "separated.aligned"
    unaligned to "separated.unaligned"
    headers   to "separated.header"
EndText

    exit 1;
}
################################################################################
