#!/usr/local/bin/perl -w
################################################################################
use strict;
use FindBin qw($Bin);
my $cmd = "$Bin/sam-diff.pl --type unaligned";
$cmd .= " '$_'" foreach (@ARGV);
exit system($cmd) >> 8;
