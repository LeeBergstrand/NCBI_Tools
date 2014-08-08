#!/usr/local/bin/perl -w
################################################################################
use strict;
use FindBin qw($Bin);
require "$Bin/common.pl";
use File::Basename "basename";
use Getopt::Long "GetOptions";
my %options;
Help()
    unless (GetOptions(\%options, "cg=s", "in=s", "help", "ncbi=s", "type=s"));
Help() if ($options{help});
Help() unless ($options{cg} && $options{ncbi} && $options{type});
Help() unless ($options{type} eq 'aligned' || $options{type} eq 'unaligned'
    || $options{type} eq 'unaligned-unmapped');
$options{in} = '.' unless ($options{in});
die "$options{in} is not a directory" unless (-d $options{in});
my $ONCE;
my $LINES # = 1
; my ($N_CG, $N_NCBI);
my $diff = 0;
unless ($options{cg} =~ /\*/) {
    Help() if ($options{ncbi} =~ /\*/);
    $diff = Diff("$options{in}/$options{cg}", "$options{in}/$options{ncbi}");
} else {
    my $ncbi = $options{ncbi};
    Help() unless ($ncbi =~ /^(.+)\*(.*)$/);
    my $ncbi_prefix = $1;
    my $ncbi_suffix = $2;
    my $cg   = $options{cg};
    die unless ($cg =~ /^(.+)\*(.*)$/);
    my $cg_prefix = $1;
    my $cg_suffix = $2;
    Help() unless ($ncbi && $cg);
    chdir $options{in} or die "cannot cd $options{in}";
    my @ncbi = glob($ncbi);
    my @cg   = glob($cg);
    opendir(DIR, '.') or die "cannot opendir $options{in}";
    my (%file_cg, %file_ncbi);
    while ($_ = readdir(DIR)) {
        next if (/^\.{1,2}$/);
        if (/^$cg_prefix(.+)$cg_suffix$/) {
            die $_ if ($file_cg{$1});
            $file_cg{$1} = $_;
        } elsif (/^$ncbi_prefix(.+)$ncbi_suffix$/) {
            die $_ if ($file_ncbi{$1});
            $file_ncbi{$1} = $_;
        } else {
            die $_;
        }
    }
    closedir DIR;
    die if (keys %file_cg != keys %file_ncbi);
    die if (@ncbi != @cg);
    die if (keys %file_cg != @cg);
    foreach my $sfx (keys %file_cg) {
        die $file_cg{$sfx} unless ($file_ncbi{$sfx});
        $diff |= Diff($file_cg{$sfx}, $file_ncbi{$sfx});
    }
}
exit($diff != 0);

sub Diff {
    my ($file_cg, $file_ncbi) = @_;
    my $diff = 0;
    print "==> CG: $file_cg\tNCBI: $file_ncbi  <==\n";
    open FILE_CG, $file_cg or die "cannot open $file_cg";
    open FILE_NCBI, $file_ncbi or die "cannot open $file_ncbi";
    my $n = 0;
    ($N_CG, $N_NCBI) = (0, 0);
    foreach my $line_cg (<FILE_CG>) {
        ++$N_CG;
        my $line_ncbi;
        next if ($line_cg =~ /^@/);
        last if ($LINES && ++$n > $LINES);
        while (1) {
            $line_ncbi = <FILE_NCBI>;
            ++$N_NCBI;
            last unless ($line_ncbi =~ /^@/);
        }
        $diff |= DiffLine($line_cg, $line_ncbi);
    }
    return $diff;
}

sub DiffLine {
    my ($line_cg, $line_ncbi) = @_;
    my %fld_cg = ParseLine($line_cg);
    my %fld_ncbi = ParseLine($line_ncbi);
    my @fields
 = qw(QNAME FLAG RNAME POS MAPQ CIGAR RNEXT PNEXT TLEN SEQ QUAL RG GS GC GQ NM);
    @fields = qw(RNAME POS MAPQ       RNEXT PNEXT      SEQ QUAL); # TLEN CIGAR
    @fields = qw(RNAME FLAG POS MAPQ                   SEQ QUAL);
    if ($options{type} eq 'aligned') {
        @fields = qw(RNAME POS MAPQ                    SEQ QUAL    GS GC GQ);
    } elsif ($options{type} eq 'unaligned') {
        @fields = qw(FLAG  MAPQ CIGAR       PNEXT TLEN SEQ QUAL    GS GC GQ NM);
    } elsif ($options{type} eq 'unaligned-unmapped') {
  @fields = qw(FLAG RNAME POS MAPQ CIGAR RNEXT PNEXT TLEN SEQ QUAL GS GC GQ NM);
        unless ($ONCE) {   ++$ONCE; } # print "@fields\n";
    }
    my $diff = 0;
    foreach (@fields) {
        $diff |= DiffFld(f_cg => $fld_cg{$_}, f_ncbi => $fld_ncbi{$_},
            key => $_, l_cg => $line_cg, l_ncbi => $line_ncbi, diff => $diff);
    }
    print "\n" if ($diff);
    return $diff;
}

sub DiffFld {
    my (%prm) = @_;
    my ($fld_cg   , $fld_ncbi  , $key    , $line_cg , $line_ncbi , $was_diff) = 
       ($prm{f_cg},$prm{f_ncbi},$prm{key},$prm{l_cg},$prm{l_ncbi},$prm{diff});
    my $diff = 0;
    if (! defined $fld_cg && ! defined $fld_ncbi) {
#       PrintDiff("> $key", $line_cg, $line_ncbi, $was_diff);
        #++$diff;
    } elsif (! defined $fld_cg) {
        PrintDiff("NCBI> $key : $fld_ncbi", $line_cg, $line_ncbi, $was_diff);
        ++$diff;
    } elsif (! defined $fld_ncbi) {
        PrintDiff("CG  > $key : $fld_cg", $line_cg, $line_ncbi, $was_diff);
        ++$diff;
    } elsif ($fld_cg ne $fld_ncbi) {
        PrintDiff
            ("<> $key : $fld_cg\t$fld_ncbi", $line_cg, $line_ncbi, $was_diff);
        ++$diff;
    } else {
#       PrintDiff "+ $key : $fld_cg\t$fld_ncbi", $line_cg, $line_ncbi;
    }
    return $diff;
}

sub PrintDiff {
    my ($diff, $line_cg, $line_ncbi, $was_diff) = @_;
    unless ($was_diff) {
        print "CG  :$N_CG $line_cg";
        print "NCBI:$N_NCBI $line_ncbi";
    }
    print "$diff\n";
}

sub Help {
    my $name = basename($0);
    print STDERR @_ if (@_);
    print STDERR <<EndText;
Usage:
    $name --cg <cg-file[s]> --ncbi <ncbi-file[s]> [--in <input-dir> ]
        --type <type>

    where <type> is aligned, unaligned, or unaligned-unmapped

    compare <cg-file[s]> and <ncbi-file[s]>

    $name --in <dir> --cg 'suffix1*' --ncbi 'suffix2*' --type <type>
    compare all pairs of "suffix1*filtered" files found in <dir>
EndText

    exit 1;
}
