#!/usr/bin/perl -w
# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================

# add parameter validation here


open SAMIN, "-|", "$ENV{NCBI}/samtools/bin/samtools view $ARGV[0]" or die "open sam from bam failed";

$_ = $ARGV[0];

print "$_\n";
s/\.bam$//;
print "$_\n";
s/^.*\///;
print "$_\n";

mkdir $_.$$ or die $!;
chdir $_.$$;

open SAMOUT, ">SAMOUT" or die $!;


while (($_ = <SAMIN>), defined($_)) {
  chomp;

  my @fields = split /\t/,$_;

  print { SAMOUT } "$fields[9]\t$fields[10]\t$fields[5]\n";
}

close SAMIN;
close SAMOUT;


