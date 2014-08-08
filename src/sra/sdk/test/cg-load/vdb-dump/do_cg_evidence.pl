#!/usr/local/bin/perl -w
################################################################################

use strict;
use Cwd 'abs_path';
use File::Basename 'basename';
use Getopt::Long "GetOptions";

sub Help {
    my ($exit) = @_;
    $exit = 0 unless (defined $exit);
    my $appname = basename($0);
    print <<EndText;
Usage:
    $appname --asm <dir with CG evidence files>
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options, "help", "asm=s"));
Help(0) if ($options{help});
Help(1) unless ($options{asm});

my (@fileIntervals, @fileDnbs);
$options{asm} = '/panfs/pan1/sra-test/golikov/loader/cg/YRI_trio.tiny/ASM'
    unless ($options{asm});
ScanDir($options{asm}, 'Intervals', 'Dnbs', \@fileIntervals, \@fileDnbs);

my $N = 0;

my $FORMAT_VERSION;
my $HEADED;
my $NO_DNB;
for (my $idx = 0; $idx <= $#fileDnbs; ++$idx) {
    $NO_DNB = 0;
    {
        $_ = $fileIntervals[$idx];
        my $cmd;
        if (/\.bz2$/) {
            $cmd = "/usr/bin/bzcat $_";
        } else {
            $cmd = "/bin/cat $_";
        }
        print STDERR "$cmd\n";
        open INTERVAL, "$cmd |" or die "cannot open $cmd";
    }
    {
        $_ = $fileDnbs[$idx];
        if ($_) {
            my $cmd;
            if (/\.bz2$/) {
                $cmd = "/usr/bin/bzcat $_";
            } else {
                $cmd = "/bin/cat $_";
            }
            print STDERR "$cmd\n";
            open DNB, "$cmd |" or die "cannot open $cmd";
        } else { ++$NO_DNB; }
    }
    my $ASSEMBLY_ID = GetASSEMBLY_ID();
    my $DNB;
    while (<INTERVAL>) {
        if (/^#/) {
            unless ($FORMAT_VERSION) {
                if (/#FORMAT_VERSION\t(.+)\n/) {
                    $FORMAT_VERSION = $1;
                }
            }
            next;
        }
        next if (/^>/);
        next if (/^$/);
#       print; next;
        chomp;
        die unless ($FORMAT_VERSION);
        my ($IntervalId, $Chromosome, $OffsetInChromosome, $Length,
            $Ploidy, $AlleleIndexes, $Score, $Allele0, $Allele1,
            $Allele2, $Allele1Alignment, $Allele2Alignment);
        my ($EvidenceScoreVAF, $EvidenceScoreEAF, $Allele3, $Allele3Alignment);
        if ($FORMAT_VERSION eq '1.5') {
            ($IntervalId, $Chromosome, $OffsetInChromosome, $Length,
             $Ploidy, $AlleleIndexes, $Score, $Allele0, $Allele1,
             $Allele2, $Allele1Alignment, $Allele2Alignment) = split(/\t/);
 #              OffsetInChromosome   Score        AlleleAlignment
        } elsif ($FORMAT_VERSION eq '2.0' || $FORMAT_VERSION eq '2.2') {
            ($IntervalId, $Chromosome, $OffsetInChromosome, $Length,
             $Ploidy, $AlleleIndexes, $EvidenceScoreVAF, $EvidenceScoreEAF,
             $Allele0, $Allele1, $Allele2, $Allele3, $Allele1Alignment,
             $Allele2Alignment, $Allele3Alignment) = split(/\t/);
            $Score = $EvidenceScoreVAF;
            $Score = $EvidenceScoreEAF if ($EvidenceScoreEAF > $Score);
        } else { die "unknown FORMAT_VERSION $FORMAT_VERSION"; }
        my $line
            = "$Chromosome\t$OffsetInChromosome\t$Length\t$Ploidy\t$Score";

        my @index;
        if ($Ploidy == 1) {
            die $_ if ($AlleleIndexes =~ /;/);
            $index[0] = $AlleleIndexes;
        } elsif ($Ploidy == 2) {
            die $_ unless ($AlleleIndexes =~ /^(\d);(\d)$/);
            @index = ($1, $2);
        } elsif ($Ploidy == 3) {
            die $_ unless ($AlleleIndexes =~ /^(\d);(\d);(\d)$/);
            @index = ($1, $2, $3);
        } else { die $_ }
#       print "$Ploidy\t$AlleleIndexes\t$Allele0\t$Allele1";
        die $_ unless (defined $index[0]);
        my %index;
        for (my $i = 0; $i < $Ploidy; ++$i) {
            ++$index{$index[$i]};
            if ($index[$i] == 0) {
                $line .= "\t$Allele0";
            } elsif ($index[$i] == 1) {
                $line .= "\t$Allele1";
            } elsif ($index[$i] == 2) {
                $line .= "\t$Allele2";
            } elsif ($index[$i] == 3) {
                die unless defined($Allele3);
                $line .= "\t$Allele3";
            } else { die $_ }
        }
#       $line .= "$Allele1Alignment";
#       $line .= "\t$Allele2Alignment" if ($Allele2Alignment);
        my @AlleleIndexes2AlleleIndex;
        my $lastAlleleIndex = 1;
        my $last = 3;
        ++$last if ($FORMAT_VERSION ne '1.5');
        for (my $i = 0; $i < $last; ++$i) {
            $AlleleIndexes2AlleleIndex[$i] = $lastAlleleIndex++
                if (defined $index{$i})
        }
        if ($Ploidy == 2 && $index[0] == $index[1]) {
            for (my $i = 0; $i < 3; ++$i) {
                $AlleleIndexes2AlleleIndex[$i] = $lastAlleleIndex
                    if (defined $index{$i});
            }
        }
        $DNB = PrintDbn($line, $IntervalId, $Chromosome, $ASSEMBLY_ID,
            $DNB, @AlleleIndexes2AlleleIndex);
    }
    die "Not all lines in DNB file are read" if ($DNB);
    close INTERVAL;
    close DNB unless ($NO_DNB);
}

sub PrintDbn {
    my ($line, $evidenceIntervalId, $evidenceChromosome, $ASSEMBLY_ID, $DNB,
        @AlleleIndexes2AlleleIndex) = @_;
    $_ = $DNB;
    unless ($NO_DNB) {
        $_ = <DNB> unless ($_);
        unless ($_) {
            print STDERR "Premature DNBS EOF\n";
            ++$NO_DNB;
        }
        else { $_ = <DNB> while (/^#/ || /^>/ || /^$/); }
    }
    my ($printed, $saved);
    unless ($HEADED) {
        print "<Chromosome\tOffsetInChromosome\tLength\tPloidy\tScore\t"
            . "READ0\tREAD1\tASSEMBLY_ID:Slide:Lane:FileNumInLane\t"
            . "REF_PLOIDY\tSide\tStrand\tOffsetInAllele\tMappingQuality\t"
            . "Sequence\tScores\n";
        ++$HEADED;
    }
    while (1) {
        if ($NO_DNB) {
            print "$line\n";
            last;
        } else {
            chomp;
            my ($IntervalId, $Chromosome, $Slide, $Lane, $FileNumInLane,
                $DnbOffsetInLaneFile, $AlleleIndex, $Side, $Strand,
                $OffsetInAllele, $AlleleAlignment, $OffsetInReference,
                $ReferenceAlignment, $MateOffsetInReference,
                $MateReferenceAlignment, $MappingQuality, $ScoreAllele0,
                $ScoreAllele1, $ScoreAllele2, $Sequence, $Scores);
            my $ScoreAllele3;
            if ($FORMAT_VERSION eq '1.5') {
               ($IntervalId, $Chromosome, $Slide, $Lane, $FileNumInLane,
                $DnbOffsetInLaneFile, $AlleleIndex, $Side, $Strand,
                $OffsetInAllele, $AlleleAlignment, $OffsetInReference,
                $ReferenceAlignment, $MateOffsetInReference,
                $MateReferenceAlignment, $MappingQuality, $ScoreAllele0,
                $ScoreAllele1, $ScoreAllele2, $Sequence, $Scores) = split(/\t/);
            } elsif ($FORMAT_VERSION eq '2.0' || $FORMAT_VERSION eq '2.2') {
               ($IntervalId, $Chromosome, $Slide, $Lane, $FileNumInLane,
                $DnbOffsetInLaneFile, $AlleleIndex, $Side, $Strand,
                $OffsetInAllele, $AlleleAlignment, $OffsetInReference,
                $ReferenceAlignment, $MateOffsetInReference,
                $MateReferenceAlignment, $MappingQuality, $ScoreAllele0,
                $ScoreAllele1, $ScoreAllele2, $ScoreAllele3, $Sequence, $Scores)
                    = split(/\t/);
            } else { die "unknown FORMAT_VERSION $FORMAT_VERSION"; }
            if ($evidenceIntervalId == $IntervalId) {
                die $_ unless ($Scores);
                die $_ unless ($evidenceChromosome eq $Chromosome);
#               if ($N == 32) {   print STDERR "" } print $N++; print "\t";
                if (defined $AlleleIndexes2AlleleIndex[$AlleleIndex]) {
                    print $line;
                    my $SPOT_GROUP = sprintf "$ASSEMBLY_ID:$Slide:$Lane:%04d", $FileNumInLane;
                    $SPOT_GROUP = "$Slide-$Lane";
                    print "\t$SPOT_GROUP";
                    print "\t$AlleleIndexes2AlleleIndex[$AlleleIndex]\t$Side";
                    print "\t$Strand\t$OffsetInAllele\t$MappingQuality\t";
                    print "$Sequence\t$Scores\n";
                    $printed = 1;
                } else {
                    $saved = 1;
                # die "$line\n$_";
                }
                $_ = <DNB>;
                unless ($_) {
                    print "$line\n" if (!$printed && $saved);
                    last;
                }
            } elsif ($printed) {
                last;
            } elsif ($saved) {
                print "$line\n";
                last;
            } elsif ($IntervalId > $evidenceIntervalId) {
                print "$line\n";
                last;
            } else
            { die "cannot find Dnb with IntervalId = $evidenceIntervalId" }
        }
    }
    return $_;
}

sub GetASSEMBLY_ID {
    return '' if ($NO_DNB);
    while (<DNB>) {
        if (/^#ASSEMBLY_ID/) {
            @_ = split;
            return $_[1];
        }
    }
    die "no #ASSEMBLY_ID in DNB";
}

sub ScanDir {
    my ($dir, $pattern1, $pattern2, $list1, $list2) = @_;
    $dir = abs_path $dir;
    die "$dir is not a directory" unless (-d $dir);
    my @files = glob("$dir/*");
    my $hasfile;
    foreach (sort @files) {
        if (-d $_) {
            ScanDir($_, $pattern1, $pattern2, $list1, $list2);
        } elsif (m|^$dir/.*$pattern1|) {
            push @$list1, $_;
            ++$hasfile;
        } elsif (m|^$dir/.*$pattern2|) {
            push @$list2, $_;
            ++$hasfile;
        } else { print STDERR "unrecognized file $_\n" }
    }
    if ($hasfile) {
        die "different number of $pattern1 and $pattern2 files in $dir"
            if (@$list1 < @$list2);
        my ($i, $j) = (0, 0);
        for (; $i <= $#$list1; ++$i, ++$j) {
            die "unrecognized reads file name "
                unless ($list1->[$i] =~ /(.*)$pattern1(.+)$/);
            my ($prefix, $suffix) = ($1, $2);
            if ($#$list2 == -1 || $list2->[$j] ne "${prefix}$pattern2$suffix") {
                splice @$list2, $j, 0, undef;
                next;
            }
        }
    }
#die"differen.number of $pattern1 and $pattern2 files in $dir"if($#$list2+1!=$j)
}
