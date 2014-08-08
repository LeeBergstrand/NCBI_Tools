#!/usr/local/bin/perl -w
################################################################################

use strict;
use Cwd 'abs_path';
use Cwd 'getcwd';
use File::Basename 'basename';
use File::Path 'mkpath';
use FindBin '$Bin';
use Getopt::Long 'GetOptions';

sub Help {
    my ($exit, $msg) = @_;
    $exit = 0 unless (defined $exit);
    my $name = basename($0);
    print "$msg\n" if ($msg);
    print <<EndText;
Usage:
$name --sra <sra run> --work <dir>
    ( --src <src> | --dir <src> | --read <file> --mapping <file> )
    [--asm <evidence-location>]
    [--clean] [--force] [--remove-files]
    --bin <cg-load binary directory>
    [--norun] [--tests <list>]
    [--help]

Examples:
    $name --sra <run path> --work <dir> --read <CG read> --mapping <CG mapping>
or
    $name --sra <run path> --work <dir> --dir <dir with CG files>

Compare CG source files and the loaded run

Options:
    --dir <dir>      directory with CG source read and mapping file[s]
    --read <file>    CG read source file
    --mapping <file> CG mapping source file
    --asm <dir>      optional parameter: evidence location directory
    --src <dir>      source directory with MAP [and ASM] subdirectories
    --sra <path>     the loaded run
    --work <path>    directory for work files
    --evidence       optional parameter: compare evidence files
    --clean          optional parameter: start by cleaning work directory
    --force          optional parameter: force existing work files overwrite
    --remove-files   optional parameter: remove files after sorting them
    --norun          optional parameter: do not execute scripts:
                                         just print command lines
    --tests <string> optional parameter:
                     test steps:
                        1 merge CG reads and mappings
                        2 cut and sort merged CG file
                        3 cg-load
                        4 vdb-dump SEQUENCE
                        5 vdb-dump ALIGNMENT-s
                        6 sort ALIGNMENT-s
                        7 merge SEQUENCE and sorted ALIGNMENT-s
                        8 cut and sort merged vdb-dump-s
                        9 compare the final CG and vdb-dumped files
                     1 => 129
                     2 => 29
                     3 => 3456789
                     4 => 4789
                     5 => 56789
                     6 => 6789
                     7 => 789
                     8 => 89
                     default: 123456789 (all)
                     
    one of --src, --dir, (--read and --mapping) is required
EndText

    exit($exit);
}

my $started = time();
#STDOUT->autoflush(1);
print STDERR `/bin/date`;
die 'date' if ($?);
print STDERR "cwd: " . getcwd() . "\n";
print STDERR "Bin: $Bin\n";
print STDERR "     $0 ";
print STDERR "@ARGV";
print STDERR "\n";

my $DBG #= 1
; my %options;
Help(1) unless (GetOptions(\%options,
    "asm=s", "bin=s", "clean", "dir=s", "force", "help", "mapping=s", "src=s",
    "norun", "read=s", "remove-files", "sra=s", "tests=s", "work=s",
    "evidence"));
Help(0) if ($options{help});
Help(1) unless ($options{sra} && $options{work});
Help(1) unless (($options{mapping} && $options{read})
    || $options{dir} || $options{src});
Help(1) unless ($options{bin});

if ($options{src}) {
    $_ = "$options{src}/MAP";
    die "$_ not found" unless (-e $_);
    $options{dir} = $_;
    
    $_ = "$options{src}/ASM";
    $options{asm} = $_ if (-e $_);
}

die "$options{bin} not found" unless (-e $options{bin});
Help(1, "--asm is required for --evidence")
    if ($options{evidence} && ! $options{asm});

unless (-e $options{work})
{   mkpath($options{work}) or die "cannot mkdir $options{work}" }
die "$options{work} is not a directory" unless (-d $options{work});

#$options{bin} = '/home/klymenka/OUTDIR/bin64' unless ($options{bin});

Abs($options{asm});
Abs($options{dir});
Abs($options{sra});
Abs($options{work});

# paths
# 1 CG input files
# 1.1   $options{dir}     # directory with native CG files
#     # or
# 1.2.1 $options{read}    #a pair of CG files (plain or bz2-ipped)
# 1.2.2 $options{mapping}
# 2 $options{sra} # loaded run

# 3 $options{work} # directory for work files.
#   It will contain:

# 3.1 Processed CG input files
# 3.1.1   all.cg  # Processed CG native files: merged CG read + mapping files
my $f_cg = "$options{work}/all.cg";
# 3.1.2   cg.last # cut and sorted cg.all
my $f_cg_last = "$options{work}/cg.last";

# 3.2          # Processed vdb-dump:

# 3.2.1.1 sequence # vdb-dump -T sequence
my $f_sequence = "$options{work}/sequence";

# 3.2.1.2.1 alignments # vdb-dump -T alignment-s
my $f_alignments = "$options{work}/alignments";

# 3.2.1.2.2 sorted alignments
my $f_alignments_sort = "$f_alignments.sort";

# 3.2.2 all.vdb # merged and processed dumps of sequence and alignments tables
my $f_dump = "$options{work}/all.vdb";

# 3.2.3 vdb.last # cut and sorted dump.all
my $f_vdb_last = "$options{work}/vdb.last";

# 3.1.2(cg.last) and 3.2.3(vdb.last) should be identical

our $T_MRG_CG; *T_MRG_CG = \(1);
my  $T_CUT_CG = 2;
my  $T_LOAD   = 3;
my  $T_VDB_SQ = 4;
my  $T_VDB_AL = 5;
my  $T_SRT_AL = 6;
my  $T_MRG_VDB= 7;
my  $T_CUT_VDB= 8;
my  $T_COMPARE= 9;
my $allTests = '123456789';
$options{tests} = $allTests unless ($options{tests});
$options{clean} = 0 if (length $options{tests} < length $allTests);
$options{clean} = 0 if ($options{norun});
my %test;
my ($threadCG, $threadVDB);
for (my $i = 0; $i < length $options{tests}; ++$i) {
    my $t = substr($options{tests}, $i, 1);
    Help(1, "'$t' : unknown tests value") unless ($allTests =~ /$t/);
    $test{$t} = 1;
    if ($t eq $T_MRG_CG) {
        $test{$T_CUT_CG} = 1;
        $test{$T_COMPARE} = 1;
        $threadCG = 1;
    } elsif ($t eq $T_CUT_CG) {
        $test{$T_COMPARE} = 1;
        $threadCG = 1;
    } elsif ($t eq $T_LOAD) {
        $test{$T_VDB_SQ} = 1;
        $test{$T_VDB_AL} = 1;
        $test{$T_SRT_AL} = 1;
        $test{$T_MRG_VDB} = 1;
        $test{$T_CUT_VDB} = 1;
        $test{$T_COMPARE} = 1;
        $threadVDB = 1;
    } elsif ($t eq $T_VDB_SQ) {
        $test{$T_MRG_VDB} = 1;
        $test{$T_CUT_VDB} = 1;
        $test{$T_COMPARE} = 1;
        $threadVDB = 1;
    } elsif ($t eq $T_VDB_AL) {
        $test{$T_SRT_AL} = 1;
        $test{$T_MRG_VDB} = 1;
        $test{$T_CUT_VDB} = 1;
        $test{$T_COMPARE} = 1;
        $threadVDB = 1;
    } elsif ($t eq $T_SRT_AL) {
        $test{$T_MRG_VDB} = 1;
        $test{$T_CUT_VDB} = 1;
        $test{$T_COMPARE} = 1;
        $threadVDB = 1;
    } elsif ($t eq $T_MRG_VDB) {
        $test{$T_CUT_VDB} = 1;
        $test{$T_COMPARE} = 1;
        $threadVDB = 1;
    } elsif ($t eq $T_CUT_VDB) {
        $test{$T_COMPARE} = 1;
        $threadVDB = 1;
    }
}

if ($options{evidence})
{   $threadVDB = $threadCG = 1 }

Help(1, '--dir should be specified when load is requested')
    if ($test{$T_LOAD} && ! $options{dir});

print STDERR "Checking the files...\n";

chdir $options{work} or die "cannot cd $options{work}";
if ($options{clean}) {
    my $date = `/bin/date`;
    die 'date' if ($?);
    chomp $date;
    print STDERR "$date\tCleaning $options{work}...\n";
    foreach (glob('*'))
    {   unlink $_ or die "cannot rm $options{work}/$_" }
    print STDERR `/bin/date`;
    die 'date' if ($?);
}

unless ($options{force} || $options{norun}) {
    die "$f_cg found"              if ($test{$T_MRG_CG} && -e $f_cg);
    die "$f_cg_last found"         if ($test{$T_CUT_CG} && -e $f_cg_last);
   #die "$options{sra} found"      if ($test{$T_LOAD  } && -e $options{sra});
    die "$f_sequence found"        if ($test{$T_VDB_SQ} && -e $f_sequence);
    die "$f_alignments found"      if ($test{$T_VDB_AL} && -e $f_alignments);
    die "$f_alignments_sort found" if ($test{$T_SRT_AL}&&-e $f_alignments_sort);
    die "$f_dump found"            if ($test{$T_MRG_VDB}&& -e $f_dump);
    die "$f_vdb_last found"        if ($test{$T_CUT_VDB}&& -e $f_vdb_last);
}

print STDERR "Starting...\n";

my $path = $options{dir}
    ? "-d $options{dir}" : "-m $options{mapping} -r $options{read}";

my $EV_CG  = "$options{work}/evidence.cg";
my $EV_VDB = "$options{work}/evidence.vdb";

my %t;
MainVdb();
if (0) {
    if (!defined(my $pid = fork)) {
        die "cannot fork: $!";
    } elsif ($pid == 0) { # child
        my $exit = DoCG();
        die $exit if ($exit);
        $exit = DoCGEvidence();
        die $exit if ($exit);
        Report();
        exit(0); # exit child
    } else { # parent
        my ($time_load, $time_aftr) = DoVDB();
        waitpid($pid, 0);
        die "CG processing failed: $!" if ($?);
        DoDiff();
        DoDiffEvidence();
        Report($time_load, $time_aftr);
    }
}

sub MainVdb {
    my $pidCg;
    if (!defined($pidCg = fork)) { # fork-1
        die "cannot fork: $!";
    } elsif ($pidCg == 0) { # child-1
        my $exit = DoCG();
        die $exit if ($exit);
        Report();
        exit(0); # exit child-1
    }
    # parent of $pidCg
    my $time_load = DoVdbLoad();
    my $t = time();
    my $pidEvidence;
    if ($options{evidence}) {
        if (!defined($pidEvidence = fork)) { # fork-2
            die "cannot fork: $!";
        } elsif ($pidEvidence == 0) { # child-2
            if (!defined(my $pid = fork)) { # fork-3
                die "cannot fork: $!";
            } elsif ($pid == 0) { # child-1's child
                my $exit = DoCGEvidence();
                die $exit if ($exit);
                Report();
                exit(0); # exit child-1'2 child
            } else { # $pid's parent
                my $exit = DoVdbEvidence();
                print STDERR "WAITing for cg evidence processing...\n";
                waitpid($pid, 0);
                die "cg evidence processing failed: $!" if ($?);
                my ($t, $exitDiff) = DoDiffEvidence() if (!$exit);
                die "Evidence diff" if ($exitDiff);
                Report();
                exit(0); # exit child-2
            }
        }
    }
    # parent of $pidEvidence
    my $pid;
    if (!defined($pid = fork)) { # fork-4
        die "cannot fork: $!";
    } elsif ($pid == 0) { # child-3
        my $exit = DoVDBSeq(); # if ($test{$T_VDB_SQ})
        die $exit if ($exit);
        Report();
        exit(0); # child-3
    }
    # $pid's parent
    my $exit = DoVDBAlg();
    print STDERR "WAITing for vdb-dump SEQUENCE...\n";
    waitpid($pid, 0);
    my $exitSeq = $?;
    my $codeSeq = $!;
    $exit = DoVDBSeqAndAlgn() unless ($exit && $exitSeq);
    print STDERR "WAITing for cg processing...\n";
    waitpid($pidCg, 0);
    my $exitCg = $?;
    my $codeCg = $!;
    $exit = DoDiff() unless ($exit && $exitSeq && $exitCg);
    if ($pidEvidence) {
        print STDERR "WAITing for evidence test to finish...\n";
        waitpid($pidEvidence, 0);
        die "evidence test failed: $!" if ($?);
    }
    die $exit if ($exit);
    die "VDB sequence processing failed: $codeSeq" if ($exitSeq);
    die "CG processing failed: $codeCg" if ($exitCg);
    $t = time() - $t;
    Report($time_load, $t);
    exit(0); # exit main
}

sub BadMainVdb {
    # MAIN THREAD
    my $pidEvidenceCg;
    if (!defined($pidEvidenceCg = fork)) { # fork-1
        die "cannot fork: $!";
    } elsif ($pidEvidenceCg == 0) { # child-1
        # MAIN'S CHILD-1 THREAD
        my $exit = DoCGEvidence();
        die $exit if ($exit);
        Report();
        exit(0); # exit child-1
    }
    # parent of $pidEvidenceCg
    my $pidCg;
    if (!defined($pidCg = fork)) { # fork-2
        die "cannot fork: $!";
    } elsif ($pidCg == 0) { # child-2
        # MAIN'S CHILD-2 THREAD
        my $exit = DoCG();
        die $exit if ($exit);
        Report();
        exit(0); # exit child-2
    }
    # parent of $pidCg
    my $time_load = DoVdbLoad();
    my $t = time();
    my $pidEvidenceVdb;
    if (!defined($pidEvidenceVdb = fork)) { # fork-3
        die "cannot fork: $!";
    } elsif ($pidEvidenceVdb == 0) { # child-3
        my $exit = DoVdbEvidence();
        print STDERR "WAITing for cg evidence dump...\n";
        waitpid($pidEvidenceCg, 0);
        die "cg evidence processing failed: $!" if ($?);
        die $exit if ($exit);
        my ($t, $exitDiff) = DoDiffEvidence();
        die "Evidence diff" if ($exitDiff);
        Report();
        exit(0); # exit child-3
    }
    # parent of $pidEvidenceVdb
    my $pid;
    if (!defined($pid = fork)) { # fork-4
        die "cannot fork: $!";
    } elsif ($pid == 0) { # child-4
        my $exit = DoVDBSeq(); # if ($test{$T_VDB_SQ})
        die $exit if ($exit);
        Report();
        exit(0); # child-4
    }
    # parent of $pid
    my $exit = DoVDBAlg();
    print STDERR "WAITing for vdb-dump SEQUENCE...\n";
    waitpid($pid, 0);
    my $exitChld = $?;
    my $errChld = $!;
    $exit = DoVDBSeqAndAlgn() unless ($exit || $exitChld);
    print STDERR "WAITing for CG read & mapping processing...\n";
    waitpid($pidCg, 0);
    my $diffExit;
    $diffExit = DoDiff() unless ($exitChld && $exit);
    print STDERR "WAITing for evidence test to complete...\n";
    waitpid($pidEvidenceVdb, 0);
    die "evidence test failed: $!" if ($?);
    die "difference found" if ($diffExit);
    die "VDB sequence processing failed: $errChld" if ($exitChld);
    die $exit if ($exit);
    $t = time() - $t;
    Report($time_load, $t);
    exit(0); # exit THE main parent
}

sub MainVdbEvidence {
    # MAIN THREAD
    my $pidEvidenceCg;
    if (!defined($pidEvidenceCg = fork)) { # fork-1
        die "cannot fork: $!";
    } elsif ($pidEvidenceCg == 0) { # child-1
        # MAIN'S CHILD-1 THREAD
        my $exit = DoCGEvidence();
        die $exit if ($exit);
        Report();
        exit(0); # exit child-1
    }
    # parent of $pidEvidenceCg
    my $pidCg;
    if (!defined($pidCg = fork)) { # fork-2
        die "cannot fork: $!";
    } elsif ($pidCg == 0) { # child-2
        # MAIN'S CHILD-2 THREAD
        my $exit = DoCG();
        die $exit if ($exit);
        Report();
        exit(0); # exit child-2
    }
    # parent of $pidCg
    my $time_load = DoVdbLoad();
    my $pidVdb;
    if (!defined($pidVdb = fork)) { # fork-3
        die "cannot fork: $!";
    } elsif ($pidVdb == 0) { # child-3
        # MAIN'S CHILD-3 THREAD
        if (!defined(my $pid = fork)) { # fork-4
            die "cannot fork: $!";
        } elsif ($pid == 0) { # child-3's child
            my $exit = DoVDBSeq(); # if ($test{$T_VDB_SQ})
            die $exit if ($exit);
            Report();
            exit(0); # child-3's child
        } else { # parent = child-3
            my $exit = DoVDBAlg();
            print STDERR "waiting for vdb-dump SEQUENCE...\n";
            waitpid($pid, 0);
            my $exitChld = $?;
            my $codeChld = $!;
            $exit = DoVDBSeqAndAlgn() unless ($exit || $exitChld);
            print STDERR "waiting for CG read & mapping processing...\n";
            waitpid($pidCg, 0);
            die "CG processing failed: $!" if ($?);
            die "VDB sequence processing failed: $codeChld" if ($exitChld);
            die $exit if ($exit);
            DoDiff();
            Report();
            exit(0); # exit child-3
        }
    }
    # parent of $pidVdb
    my $exit = DoVdbEvidence();
    print STDERR "waiting for evidence vdb-dump...\n";
    waitpid($pidEvidenceCg, 0);
    die "vdb-dump evidence failed: $!" if ($?);
    my ($t, $exitDiff) = DoDiffEvidence() if (!$exit);
    print STDERR "waiting for seq/quality comparing...\n";
    waitpid($pidVdb, 0);
    die "VDB processing failed: $!" if ($?);
    die "Evidence diff" if ($exitDiff);
    Report($time_load, $t);
    exit(0); # exit THE main parent
}

sub DoCG {
    my $exit;
    if ($test{$T_MRG_CG}) { # 1
        if (glob("$options{work}/cg.*")) {
            $exit = "$options{work}/cg.* found"
                unless ($options{force} || $options{norun})
        }
        unless ($exit) {
            my $cmd = "$Bin/merge_n_check_cg_files.pl $path -s SKIP"
                  . " PIPE $Bin/split.pl -f 5 -o $options{work}/cg.";
            $cmd = "$Bin/cg_merge_n_split.sh $Bin $options{dir}"
                                             . " $options{work}/cg."
                                             . " $options{bin}";
            $exit = Run("Merging CG read and mapping files", $cmd);
        }
    }
    if (! $exit && $test{$T_CUT_CG}) { # 2
        print STDERR "export TMPDIR=$options{work}\n";
        $ENV{TMPDIR} = $options{work};
#       die "$f_cg not found" unless (-e $f_cg);
        $exit = Run("Cutting and sorting CG file", "$Bin/cut_n_sort.pl -f 1-10 "
            . " -i $options{work}/cg. -o $options{work}/sort.cg.");
    }
    return $exit;
}

sub DoCGEvidence {
    my $exit;
    if (! $exit && $options{evidence}) {
        $exit = Run("Extracting CG evidence",
            "$Bin/do_cg_evidence.pl -a $options{asm} > $EV_CG");
    }
    return $exit;
}

sub DoVDB {
    my $time_load = DoVdbLoad();
    my $time_aftr = DoVDBAfterLoad
        ($test{$T_VDB_SQ} && ($test{$T_VDB_AL} || $test{$T_SRT_AL}));
    return ($time_load, $time_aftr);
}

sub DoVdbLoad {
    my $time_load;
    if ($test{$T_LOAD}) { # 3
        $time_load = time();
        my $cmd
            = "$Bin/load.pl -b $options{bin} -m $options{dir} -o $options{sra}";
        $cmd .= " -a $options{asm}" if ($options{asm});
        my $exit = Run("Running cg-load", $cmd);
        die $exit if ($exit);
        $time_load = time() - $time_load;
    }
    return $time_load;
}

sub DoVDBAfterLoad {
    my ($fork) = @_;
    my $t = time();
    if (!defined(my $pid = fork)) {
        die "cannot fork: $!";
    } elsif ($pid == 0) { # child
        if (!defined(my $pid = fork)) {
            die "cannot fork: $!";
        } elsif ($pid == 0) { # child
            my $exit = DoVDBSeq(); # if ($test{$T_VDB_SQ})
            die $exit if ($exit);
            Report();
            exit(0); # exit child
        } else { # parent
            my $exit = DoVDBAlg(); # if ($test{$T_VDB_AL} or $test{$T_SRT_AL})
            waitpid($pid, 0);
            die "CG processing failed: $!" if ($?);
            die $exit if ($exit);
            $exit = DoVDBSeqAndAlgn();
            die $exit if ($exit);
            Report();
            exit(0); # exit child
        }
    } else { # parent
        my $exit;
        $exit = DoVdbEvidence() if ($options{evidence});
        waitpid($pid, 0);
        die "CG processing failed: $!" if ($?);
        die $exit if ($exit);
        $t = time() - $t;
        return $t;
    }
}

sub DoVdbEvidence {
    my $exit;
    return 0 unless ($options{evidence});
    return Run("Dumping VDB evidence",
        "$Bin/vdb_dump_EVIDENCE.pl "
        . "-b $options{bin} -s $options{sra} > $EV_VDB");
}

sub DoVDBSeq {
    my $exit;
    if ($test{$T_VDB_SQ}) { # 4
        $exit = Run("Dumping SEQUENCE", "$Bin/vdb_dump_sequence.pl "
            . "-s $options{sra} -b $options{bin} > $f_sequence");
        
    }
    return $exit;
}

sub DoVDBAlg {
    my $exit;
    if ($test{$T_VDB_AL}) { # 5
        $exit = Run("Dumping ALIGNMENT-s", "$Bin/vdb_dump_alignments.pl"
            . " -s $options{sra} -b $options{bin} > $f_alignments");
    }
    if (! $exit && $test{$T_SRT_AL}) { # 6
        print STDERR "export TMPDIR=$options{work}\n";
        $ENV{TMPDIR} = $options{work};
        $exit = Run("Sorting ALIGNMENT-s",
            "/bin/sort -k3,3n -k4,4 $f_alignments > $f_alignments_sort");
        if (! $exit && $options{'remove-files'})
        {   unlink($f_alignments) or die "cannot remove $f_alignments" }
    }
    return $exit;
}

sub DoVDBSeqAndAlgn {
    my $exit;
    if ($test{$T_MRG_VDB}) { #7
        my $cmd = "$Bin/merge_vdb_dumps.pl -s $f_sequence -a $f_alignments_sort"
             . " PIPE $Bin/split.pl -f 12 -o $options{work}/vdb.";
        $cmd = "$Bin/vdb_merge_n_split.sh $Bin $f_sequence   $f_alignments_sort"
                                         . " $options{work}/vdb.";
        $exit = Run("Merging SEQUENCE and ALIGNMENT-s", $cmd);
        if (! $exit && $options{'remove-files'}) {
            unlink($f_sequence, $f_alignments_sort)
                or die "cannot rm $f_sequence $f_alignments_sort"
        }
    }
    if (! $exit && $test{$T_CUT_VDB}) { #8
        print STDERR "export TMPDIR=$options{work}\n";
        $ENV{TMPDIR} = $options{work};
        $exit = Run("Cutting and sorting dump file", "$Bin/cut_n_sort.pl "
           . "-f 2-4,11-17 -i $options{work}/vdb. -o $options{work}/sort.vdb.");
    }
    return $exit;
}

sub DoDiff {
    my $exit;
    if ($test{$T_COMPARE}) { #9
        #unless (-e $f_cg_last && -e $f_vdb_last) {
         #   die "$f_cg_last or $f_vdb_last not found" unless ($options{norun});
        $exit = Run("Comparing the files", "$Bin/diff.pl "
            . "-1 $options{work}/sort.cg. -2 $options{work}/sort.vdb.");
    }
    return $exit;
}

sub DoDiffEvidence {
    my $exit;
    my $t = time();
    if ($options{evidence}) {
        $exit = Run("Comparing evidence files",
            "/usr/bin/diff $EV_CG $EV_VDB");
    }
    $t = time() - $t;
    return ($t, $exit);
}

sub Run {
    my $exit = 0;
    my $t = time();
    my ($msg, $cmd) = @_;
    my $date = `/bin/date`;
    die 'date' if ($?);
    chomp $date;
    print STDERR "\n> $date\t$msg\n";
    print STDERR "$cmd\n";
    unless ($options{norun}) {
        $exit = system($cmd);
        $date = `/bin/date`;
        die 'date' if ($?);
        chomp $date;
        $t = time() - $t;
        Time($msg, $t);
        print STDERR "< $date\tDone $msg in " . MkTime($t) . "\n";
    }
    return $cmd if ($exit);
    return 0;
}

sub Report {
    my ($time_load, $time_aftr) = @_;
    my $short;
    ++$short unless ($time_load || $time_aftr);
    my @out;
    foreach ($options{sra}, $options{work}) {
        my $cmd = m|/panfs/| ? '/opt/panfs/bin/pan_df' : '/bin/df';
        my $out = `$cmd -h $_`;
        die $cmd if ($?);
        push @out, $out;
    }
    my $last;
    unless ($short) {
        print "\n";
        foreach (@out) {
            next if ($last && $_ eq $last);
            print;
            $last = $_;
        }
    }
    my $dt = time() - $started;
    my $t  = MkTime($dt);
    my $l = length($t);
    my ($t1, $t2);
    if ($time_load) {
        $t1 = MkTime($time_load);
        $l = length($t1) if (length($t1) > $l);
    }
    if ($time_aftr) {
        $t2 = MkTime($time_aftr);
        $l = length($t2) if (length($t2) > $l);
    }
    unless ($short) {
        printf(STDERR "\nSUCCESS: ");
        printf(STDERR $options{src} ? $options{src} : $options{dir});
        printf(STDERR "\nDone in         %.*s\n", $l, $t );
        printf(STDERR   "Load took       %.*s\n", $l, $t1) if ($time_load);
        printf(STDERR   "VDB dump&merge: %.*s\n", $l, $t2) if ($time_aftr);
        print STDERR "\n";
    }
    printf(STDERR "%7s\t$t{$_}\n", MkTime(int $_))
        foreach (sort {$b <=> $a} keys %t);
}

sub MkTime {
    my ($t) = @_;
    my $s = $t % 60;
    my $mm = ($t - $s) / 60;
    return sprintf("%2d:%02d", $mm, $s) if ($mm < 60);
    my $m = $mm % 60;
    my $h = ($mm - $m) / 60;
    return sprintf("$h:%02d:%02d", $m, $s);
}

sub Time {
    my ($name, $t) = @_;
    $t += .1 while ($t{$t});
    $t{$t} = $name;
}

sub Abs { $_[0] = abs_path($_[0]) if (defined $_[0]) }

################################################################################
# EOF #
