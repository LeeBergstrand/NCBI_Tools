#!/usr/local/bin/perl -w
################################################################################

use strict;

=begin COMMENT
if MODE_DIFF_SEQ: vdb-dump SEQUENCE, compare it with CG reads file
if MODE_MERGE   : compare vdb-dump SEQUENCE with CG reads file;
                  print merged CG reads and mapping file
  unless (WITH_WITHOUT_MAPPING) {
      the output file contains just reads that have mappings!
      i.e. reads without mappings were skipped
  } else { it containg reads without mappings, as well }

command line agruments:
--read    # path to input CG read    file
--mapping # path to input CG mapping file
--sra     # path to the loaded run
=cut
################################################################################

use Cwd 'abs_path';
use File::Basename "basename";
use Getopt::Long "GetOptions";

sub println { print @_; print "\n" }

my $appname = basename($0);

sub Help {
    my ($exit) = @_;
    $exit = 0 unless (defined $exit);
    print <<EndText;
Usage:
> $appname --read<CG read file> --mapping <CG mapping file> --sra <run path>
Merge read and mapping file; compare it with sra dump

> $appname --read<CG read file> --mapping <CG mapping file> --sra SKIP
Merge CG files; skip "vdb-dump -T SEQUENCE" and subsequent compare

> $appname --dir<dir with CG read and mapping files> --sra <run path>
> $appname --dir<dir with CG read and mapping files> --sra SKIP
Process all CG files found in directory

> $appname --dir<dir with CG read and mapping files> --sra SKIP --bin <bin-path>
Use vdb-dump from <bin-path> directory
EndText

    exit $exit;
}

my $NO_VDB_DUMP;
my %options;
Help(1) unless (GetOptions(\%options,
    "bin=s", "help", "dir=s", "mapping=s", "read=s", "sra=s"));
Help()  if ($options{help});
Help(1) unless ($options{sra});
Help(1) unless (($options{mapping} && $options{read}) || $options{dir});
$NO_VDB_DUMP = $options{sra} eq 'SKIP';

my $MODE_DIFF_SEQ = 0;
my $MODE_MERGE    = 1;
my $FIX_FLAGS = 1;
my $WITH_WITHOUT_MAPPING = 1;
my $mode = $MODE_MERGE;
my $FIX_reads_FLAGS = 1;

my $LeftHalfDnbNoMatches    = 1;
my $LeftHalfDnbMapOverflow  = 2;
my $RightHalfDnbNoMatches   = 4;
my $RightHalfDnbMapOverflow = 8;
my $LastDNBRecord = 1;
my $flag_side     = 2;
my $flag_strand   = 4;

my $vdb_dump = '/panfs/traces01/trace_software/toolkit/centos64/bin/vdb-dump';
$vdb_dump = '/home/klymenka/OUTDIR/bin64/vdb-dump';
$vdb_dump = "$options{bin}/vdb-dump" if ($options{bin});
my $run = '/panfs/pan1/sra-test/golikov/loader/cg/SRZ-tiny';
unless ($NO_VDB_DUMP) {
    $run = $options{sra};
    my $cmd = "$vdb_dump $run -Nl0 -CALIGNMENT_COUNT,READ,QUALITY";
    open READ, "$cmd |" or die "cannot open $cmd";
}

my (@filesMapping, @filesReads);
if ($options{dir}) {
    ScanDir($options{dir}, 'mapping_', 'reads_', \@filesMapping, \@filesReads);
} else {
    my $root = '/panfs/pan1/sra-test/golikov/loader/cg/YRI_trio.tiny/MAP';
    my $read = "$root/100.reads_GS10351-FS3-L01_001.tsv.bz2";
    $read = $options{read};
    push @filesReads, $read;
    my $map  = "$root/100.mapping_GS10351-FS3-L01_001.tsv.bz2";
    $map = $options{mapping};
    push @filesMapping, $map;
}

my ($HEADER_READ, $HEADER_MAP, $HEADED) = ('', '');
my $row = 0;
for (my $fidx = 0; $fidx <= $#filesReads; ++$fidx) {
    my $read = $filesReads[$fidx];
    my $map = $filesMapping[$fidx];
    die $read unless (-f $read);
    die $map  unless (-f $map);

    if ($read =~ /\.bz2$/) {
             my $cmd = "/usr/bin/bzcat $read";
             open CG_READ, "$cmd |" or die "cannot open $cmd";
    } else { open CG_READ, $read or die "cannot open $read"; }

    if ($map =~ /\.bz2$/) {
             my $cmd = "/usr/bin/bzcat $map";
             open CG_MAP, "$cmd |" or die "cannot open $cmd";
    } else { open CG_MAP, $map or die "cannot open $map"; }

    if ($#filesReads) {
        my $date = `/bin/date`;
        die 'date' if ($?);
        chomp $date;
        print STDERR "$date\t$read\t$map\n";
    }

    my $last #= 5
    ;for (; ;) {
        last if (defined $last && $row > $last);

        my ($flags_r, $reads, $scores);
        my $eof = 1;
        while (<CG_READ>) {
            chomp;
            $eof = 0;
            if (/^#/) {
                next;     # skip headers
            }
            next if (/^$/);     # skip empty lines
            if (/^>flags\treads\tscores$/) {
                $HEADER_READ = $_;
#               print "$_\t" if ($MODE_MERGE);
                next;
            }
            ++$row;
            ($flags_r, $reads, $scores) = split /\t/;
            last;
        }
        last if ($eof);
        my $cg_read = $_;

        my ($alignment_count, $read, $quality, @alignment, @quality);
        unless ($NO_VDB_DUMP) {
            my $alignment_count = <READ>;
            my $read = <READ>;
            my $quality = <READ>;
            die "premature"
                if (($cg_read && !$quality) || (!$cg_read && $quality));
            last unless ($cg_read && $quality);
            chomp $alignment_count;
            chomp $read;
            chomp $quality;

            die $alignment_count unless ($alignment_count =~ /^(\d+), (\d+)$/);
            @alignment = ($1, $2);

            @quality = split /, /, $quality;

            die "$row\n$reads\n$read" unless ($reads eq $read);
        }

        my $noLeftMap
            = $flags_r & ($LeftHalfDnbNoMatches  | $LeftHalfDnbMapOverflow);
        my $noRightMap
            = $flags_r & ($RightHalfDnbNoMatches | $RightHalfDnbMapOverflow);
        if ($FIX_FLAGS) {
            my $flags = $flags_r & $LeftHalfDnbNoMatches;
            $flags |= $LeftHalfDnbNoMatches
                if ($flags_r & $LeftHalfDnbMapOverflow);
            $flags |= $flags_r & $RightHalfDnbNoMatches;
            $flags |= $RightHalfDnbNoMatches
                if ($flags_r & $RightHalfDnbMapOverflow);
            $flags_r = $flags;
        }

        if ($MODE_MERGE && $FIX_FLAGS && !$HEADED) {
            print ">flags\treads\tscores\tflags\tchromosome";
            print "\toffsetInChr\tgap1\tgap2\tgap3\tweight\tmateRec\n";
            $HEADED = 1;
        }
        my @map = (0, 0);
        unless ($noLeftMap && $noRightMap) {
          $flags_r = $LeftHalfDnbNoMatches | $RightHalfDnbNoMatches
            if ($FIX_reads_FLAGS);
          my @mapping;
          while (<CG_MAP>) {
            chomp;
            if (/^#/) {
                next;     # skip headers
            }
            next if (/^$/);     # skip empty lines
            if (/^>/) {
                $HEADER_MAP = $_;
#               print if ($MODE_MERGE);
                next;        
            }
            my ($flags_m, $chromosome,
                    $offsetInChr, $gap1, $gap2, $gap3, $weight, $mateRec)
                = split /\t/;
            my ($flag, $rest);
            if ($MODE_MERGE) {
                if ($FIX_FLAGS) {
                    die $_ unless (/^(.)(.+)$/);
                    ($flag, $rest) = ($1, $2);
                    $flag &= ~ $LastDNBRecord;
                    if ($FIX_reads_FLAGS) {
                        if ($flag & $flag_side) { # right
                                 $flags_r &= ~ $RightHalfDnbNoMatches;
                        } else { $flags_r &= ~ $LeftHalfDnbNoMatches ; }
                    }
                } else { print "$cg_read\t$_"; }
            }
            my $side = ($flags_m & $flag_side) ? 1 : 0;
            ++$map[$side];
            if ($MODE_MERGE && $FIX_FLAGS)
            {   push @mapping, "$flag$rest"; } # TODO
            last if ($flags_m & $LastDNBRecord);
          }
          if ($MODE_MERGE && $FIX_FLAGS) {
            print "$flags_r\t$reads\t$scores\t$_\n" foreach (@mapping);
          }
        } elsif ($WITH_WITHOUT_MAPPING) { print "$flags_r\t$reads\t$scores\n"; }
        unless ($NO_VDB_DUMP) {
            die "$row\n$alignment_count\n$flags_r"
                if (($alignment[0] && $noLeftMap) ||
                    ($alignment[1] && $noRightMap));
            die "$row\n$alignment_count\n$flags_r"
                if (($alignment[0] != $map[0]) || ($alignment[1] != $map[1]));
            die ("$row\t$quality\n" . $scores)
                if ($#quality + 1 != length $scores);

            for (my $i = 0; $i <= $#quality; ++$i) {
                my $char = ord(substr($scores, $i, 1));
                die ("$i @ $row\t$quality\n" . $scores)
                    if ($quality[$i] != ($char - 33));
                print substr($scores, $i, 1) . " == $quality[$i]\n" if (0);
            }
        }

        if (0) {
            unless ($NO_VDB_DUMP) {
                printf "%6d\t", $row;
                print "'$alignment_count'\t" . ($noLeftMap ? 0 : 1)
                    . "\t" . ($noRightMap ? 0 : 1) . "\t";
                print "$alignment[0] = $map[0]\t$alignment[1] = $map[1]\n";
                print "$quality\n$scores\t";
            }
        }

        printf "$reads\n" if ($mode == $MODE_DIFF_SEQ);
    }

    close READ unless ($NO_VDB_DUMP);
    close CG_READ;
    close CG_MAP;
}

print STDERR "$appname: $row CG read rows processed\n";

sub ScanDir {
    my ($dir, $pattern1, $pattern2, $list1, $list2) = @_;
    $dir = abs_path $dir;
    die "$dir is not a directory" unless (-d $dir);
    my @files = glob("$dir/*");
    foreach (sort @files) {
        if (-d $_) {
            ScanDir($_, $pattern1, $pattern2, $list1, $list2);
        } elsif (m|^$dir/.*$pattern1|) {
            push @$list1, $_;
        } elsif (m|^$dir/.*$pattern2|) {
            push @$list2, $_;
        } else { die "unrecognized file $_" }
    }
    die "different number of reads and mapping files in $dir"
        unless (@$list1 == @$list2);
    for (my $i = 0; $i <= $#$list1; ++$i) {
        die "unrecognized reads file name "
            unless ($list1->[$i] =~ /(.*)$pattern1(.+)$/);
        my ($prefix, $suffix) = ($1, $2);
        my $cand = "${prefix}$pattern2$suffix";
        unless ($list2->[$i] eq $cand) {
            $cand = $1 if ($cand =~ /^(.+).bz2$/);
            die "cannot find matching mapping file for $list1->[$i]"
                unless ($list2->[$i] eq $cand);
        }
    }
}

################################################################################
# EOF #
