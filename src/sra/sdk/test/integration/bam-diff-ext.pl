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
use IO::File;

sub usage() {
  print "Usage:\n";
  print "  sam-diff.pl <DIR> [-DEBUG]\n\n";
  print "Summary:\n";
  print "  Find the files created by scripts sam-tools-dump.pl and\n";
  print "  vdb-tools-dump.pl and display any differences.\n\n";
  print "Parameters:\n";
  print "  <DIR> is the directory to start our work in.\n";
  print "    This directory must already exist and be populated.\n\n";
  print "Options:\n";
  print "  -DEBUG this will just display some messages showing progess\n";
  print "         as the script runs.\n\n";
}

# options and parameters
my $debug = 0;
my $indir;
my $index;
my @params;

for ($index = 0; $index < $#ARGV + 1; ++$index) {
  if (($ARGV[$index] eq '-debug') || ($ARGV[$index] eq '-DEBUG')) {
    $debug = 1;
  }
  else {
    push(@params, $ARGV[$index]);
  }
}

# we require and accept only a single parameter
if ($#params < 0) {
  print ("too few parameters\n");
  usage();
  exit(3);
}
elsif ($#params > 0) {
  print ("too many parameters\n");
  usage();
  exit(3);
}
$indir = $params[0];

if ($debug) {
  print ("supplied input directory $indir\n");
}

# validate directory structure
unless (-d $indir) {
  print { STDERR } "OUT-DIR parameter not a usable directory\n";
  usage ();
  exit 3;
}
chdir ("$indir") or die "can't use $indir as a directory";

my @required_directories = ("samtools","vdbtools","samtools/aligned","vdbtools/aligned","samtools/unaligned","vdbtools/unaligned");

foreach (@required_directories) {
  if (! -d $_) {
    print { STDERR } ("Missing directory '$indir/$_', aborting...\n\n");
    exit(3);
  }
}


# okay we're now at home in the requested working directory...

# start logging
open (my $log, ">", "bam-diff-ext.log") or die "Cannot open bam-diff.log for output";

# build a list of the files in each of the four sub-directories
my @samtool_aligned_core_files = ();
my @samtool_aligned_ext_files = ();
my @samtool_unaligned_files = ();
my @vdb_aligned_core_files = ();
my @vdb_aligned_ext_files = ();
my @vdb_unaligned_files = ();

chdir ("samtools");
@samtool_aligned_core_files = <aligned/core_*>;
@samtool_aligned_ext_files = <aligned/ext_*>;
@samtool_unaligned_files = <unaligned/*>;
chdir ("../vdbtools");
@vdb_aligned_core_files = <aligned/core_*>;
@vdb_aligned_ext_files = <aligned/ext_*>;
@vdb_unaligned_files = <unaligned/*>;
chdir ("..");

my $diffdir = "diffs";

mkdir $diffdir;
mkdir "$diffdir/aligned";
mkdir "$diffdir/unaligned";

# look for missing files and put out a message for files
# produced by one side but not the other
foreach $file (@samtool_aligned_ext_files) {
  if (! -f "vdbtools/$file") {
    print {STDOUT} ("ERROR: missing file 'vdbtools/$file'\n");
    print {$log}   ("ERROR: missing file 'vdbtools/$file'\n");
  }
}
foreach $file (@vdb_aligned_ext_files) {
  if (! -f "samtools/$file") {
    print {STDOUT} ("ERROR: missing file 'vdbtools/$file'\n");
    print {$log}   ("ERROR: missing file 'vdbtools/$file'\n");
  }
}
# compare files that are in both the smtools and vdb tools directories
# identical files produce no output
foreach $file (@samtool_aligned_ext_files) {
  if (-f "vdbtools/$file") {
    if ($debug) {
      print ("comparing file 'samtools/$file' and 'vdbtools/$file'\n");
    }
    if (system ("ext_diff.pl samtools/$file vdbtools/$file >$diffdir/$file") == 0) {
      print {STDOUT} ("files match '$file'\n");
      print {$log}   ("files match '$file'\n");
      unlink ("$diffdir/$file");
    }
    else {
      print {STDOUT} ("ERROR: files differ see $diffdir/$file'\n");
      print {$log}   ("ERROR: files differ see $diffdir/$file'\n");
    }
  }
}


close ($log);

__END__
