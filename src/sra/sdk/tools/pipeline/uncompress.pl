#!/opt/perl-5.8.8/bin/perl -w

# TODO : the script fails if copycat-result-catalog-xml is located in unexisting directory

use strict;

use Getopt::Long;

sub Usage {
  print STDERR "Usage: $0 --copycat <copycat_path> [--help] --out <output_dir> --src <src-file> --xml <copycat-result-catalog-xml>\n";
  exit(1);
}

my %options;

Usage() unless (GetOptions(\%options, "copycat=s", "help", "src=s", "out=s", "xml=s"));
Usage() if ($options{help});
Usage() unless ($options{copycat} && $options{out} && $options{src} && $options{xml});

my $cmd = "$options{copycat} '$options{src}' /dev/null -e $options{out} -E -X > $options{xml}";
print STDERR "$cmd\n";

my $res = system($cmd);
exit($res >> 8);
