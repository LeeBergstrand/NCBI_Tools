#!/usr/local/bin/perl -w
################################################################################

use strict;
use File::Basename "basename";
use Getopt::Long "GetOptions";

my $PRINT_INTERVAL_SPOT_ID = 0;

sub Help {
    my ($exit) = @_;
    $exit = 0 unless (defined $exit);
    my $name = basename($0);
    print <<EndText;
Usage:
    $name --sra <run path> --bin <vdb-dump binary directory>
    
    Dump EVIDENCE table while converting it into intermediate CG-like format
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options, "bin=s", "help", "sra=s"));
Help(0) if ($options{help});
Help(1) unless ($options{sra});

my $run = '/home/klymenka/TEST_CG/RUN';
$run = $options{sra};
my $vdb_dump = '/panfs/traces01/trace_software/toolkit/centos64/bin/vdb-dump';
$vdb_dump = '/home/klymenka/OUTDIR/bin64/vdb-dump';
$vdb_dump = "$options{bin}/vdb-dump" if ($options{bin});

my $hasAlignment = HasAlignment();

my @colALIGNMENT = qw(MAPQ REF_ID REF_NAME REF_ORIENTATION REF_PLOIDY REF_POS
    SEQ_READ_ID SEQ_SPOT_ID SPOT_GROUP);
if ($hasAlignment) {
    my $cmd = MkCmd('EVIDENCE_ALIGNMENT', @colALIGNMENT);
    print STDERR "$cmd\n";
    open ALIGNMENT, "$cmd |" or die "cannot open $cmd";
}
my @colINTERVAL = qw(CIGAR_SHORT CIGAR_SHORT_LEN
    MAPQ READ READ_LEN READ_START REF_ID REF_LEN REF_POS SPOT_ID);
{
    my $cmd = MkCmd('EVIDENCE_INTERVAL', @colINTERVAL);
    print STDERR "$cmd\n";
    open INTERVAL, "$cmd |" or die "cannot open $cmd";
}
my $HEADED;
my $ALIGNMENT;
while (<INTERVAL>) {
    chomp;
    %_ = Split($_, @colINTERVAL);
    my @cigarShortLen = split /, /, $_{CIGAR_SHORT_LEN};
    my @cigarShort;
    {   my $p = 0;
        foreach my $l(@cigarShortLen) {
            push @cigarShort, substr($_{CIGAR_SHORT}, $p, $l);
            $p += $l;
    }   }
    my @readLen = split /, /, $_{READ_LEN};
    my @readStart = split /, /, $_{READ_START};
    die $_ unless ($#readLen == $#readStart);
    die $_ unless ($#readLen == $#cigarShortLen);
    my @read;
    {   my $p = 0;
        for (my $i = 0; $i <= $#readLen; ++$i) {
            die $_ unless ($readStart[$i] == $p);
            push @read, substr($_{READ}, $p, $readLen[$i]);
            $p += $readLen[$i];
        }
        die $_ unless (length($_{READ}) == $p);
    }
    if ($#readLen == 1 || $#readLen == 2) {
#die $_ unless ($cigarShort[0] eq $cigarShort[1] && $cigarShortLen[0] == $cigarShortLen[1] && $readLen[0] == $readLen[1]);
    } elsif ($#readLen != 0) { die $_ }
    my $cmd = "$vdb_dump -CNAME -Nl0 -TREFERENCE $run -R$_{REF_ID}";
    my $NAME = `$cmd`;
    die $cmd if ($?);
    chomp $NAME;
    my $line = "$NAME\t$_{REF_POS}\t$_{REF_LEN}\t"
        . ($#readLen + 1) . "\t$_{MAPQ}";
    $line .= "\t$_" foreach (@read);
#   $line .= "\t$cigarShort[0]";
    $ALIGNMENT = Print($line, $_{SPOT_ID}, $NAME, $ALIGNMENT);
}
die "Not all lines from EVIDENCE_ALIGNMENT are read" if ($ALIGNMENT);
close ALIGNMENT if ($hasAlignment);
close INTERVAL;

sub Print {
    my ($line, $INTERVAL_SPOT_ID, $INTERVAL_REF_NAME, $ALIGNMENT) = @_;
    $_ = $ALIGNMENT;
    if ($hasAlignment)
    {   $_ = <ALIGNMENT> unless ($_); }
    my $printed;
    unless ($HEADED) {
        print "<Chromosome\tOffsetInChromosome\tLength\tPloidy\tScore\t"
            . "READ0\tREAD1\tASSEMBLY_ID:Slide:Lane:FileNumInLane\t"
            . "REF_PLOIDY\tSide\tStrand\tOffsetInAllele\tMappingQuality\t"
            . "Sequence\tScores\n";
        ++$HEADED;
    }
    while (1) {
        %_ = Split($_, @colALIGNMENT);
        if ($INTERVAL_SPOT_ID == $_{REF_ID}) {
			print "$INTERVAL_SPOT_ID\t" if ($PRINT_INTERVAL_SPOT_ID);
            die $_ unless ($INTERVAL_REF_NAME eq $_{REF_NAME});
            my $cmd
                = "$vdb_dump -CQUALITY,READ -ftab -l0 -R$_{SEQ_SPOT_ID} $run";
            my $SEQUENCE = `$cmd`;
            chomp $SEQUENCE;
            die $cmd if ($?);
            my ($QUALITY, $READ) = split(/\t/, $SEQUENCE);
            my @quality = split /, /, $QUALITY;
            my $Side;
            if      ($_{SEQ_READ_ID} == 1) {
                $Side = 'L';
            } elsif ($_{SEQ_READ_ID} == 2) {
                $Side = 'R';
            } else { die $_ }
            my $Strand;
            if ($_{REF_ORIENTATION} eq 'false') {
                $Strand = '+'
            } elsif ($_{REF_ORIENTATION} eq 'true') {
                $Strand = '-'
            } else { die $_ }
            print "$line\t$_{SPOT_GROUP}\t$_{REF_PLOIDY}" .
                "\t$Side\t$Strand\t$_{REF_POS}\t";
            print chr($_{MAPQ} + 33);
            print "\t$READ\t";
            print chr($_ + 33) foreach (@quality);
            print "\n";
            $printed = 1;
            $_ = <ALIGNMENT>;
            last unless ($_);
        } elsif ($printed) {
            last;
        } else {
			print "$INTERVAL_SPOT_ID\t" if ($PRINT_INTERVAL_SPOT_ID);
            if (0) {
              print STDERR
                "INFO: cannot find ALIGNMENT with REF_ID = $INTERVAL_SPOT_ID\n";
            }
            print "$line\n";
            last;
        }
    }
    return $_;
}

sub MkCmd {
    my ($table, @col) = @_;
    my $cmd = "$vdb_dump -ftab -l0 -T$table $run";
    my $col = '';
    foreach (@col) {
        $col .= ',' if ($col);
        $col .= $_;
    }
    $cmd .= " -C$col" if ($col);
}

sub Split {
    my ($l, @col) = @_;
    $_ = $l;
    chomp;
    @_ = split /\t/;
    die $_ if (@_ != @col);
    my %col;
    for (my $i = 0; $i <= $#_; ++$i) {
        die $col[$i] if ($col{$col[$i]});
        $col{$col[$i]} = $_[$i];
    }
    return %col;
}

sub HasAlignment {
    my $cmd = "$vdb_dump -E $options{sra}";
    open TBLS, "$cmd |" or die "cannot open $cmd";
    foreach (<TBLS>)
    {   return 1 if (/: EVIDENCE_ALIGNMENT$/); }
    return 0;
}

################################################################################
# EOF #
