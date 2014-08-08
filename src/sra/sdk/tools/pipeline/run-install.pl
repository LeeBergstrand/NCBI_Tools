#!/opt/perl-5.8.8/bin/perl -w

use strict;

use Getopt::Long;

use FindBin qw($Bin);
require "$Bin/common.pl";

sub Usage
{ print STDERR "Usage: $0 --bin <sra_bin_dir> --out <output_path> [--extra_args <loaded-extra-command-line-args>] --src <src-file>\n"; exit(1); }

my %options;
Usage() unless (GetOptions(\%options, "extra_args=s", "help", "bin=s", "out=s", "src=s"));
Usage() if ($options{help});
Usage() unless ($options{bin} && $options{out} && $options{src});

$options{extra_args} = '' unless $options{extra_args};

SetEnv();

my $rc;
{
    my $cmd = "$options{bin}/sra-dbcc -5 -b $options{src}";
    LogExec($cmd);
    $rc = system($cmd);
    Error("failed to run sra-dbcc") if ($rc);
}
unless ($rc) {
    my $cmd = "$options{bin}/vdb-copy $options{extra_args} $options{src} $options{out}";
    LogExec($cmd, 'noClose');
    $rc = system($cmd);
    Error("failed to run vdb-copy") if ($rc);
    print "</Exec>\n";
}
exit($rc >> 8);
