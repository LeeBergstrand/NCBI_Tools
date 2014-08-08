#!/usr/local/bin/perl -w
################################################################################

use strict;
use File::Basename "basename";
use Getopt::Long "GetOptions";

sub Help {
    my ($exit) = @_;
    $exit = 0 unless (defined $exit);
    my $name = basename($0);
    print <<EndText;
Usage:
    $name <OPTIONS>

    Truncate CG native files

Options:
    --read <CG read file>
    --mapping <CG mapping file>
    --output <output directory>
    --lines <N>                 print the first N lines of read file
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options,
    "help", "lines=n", "mapping=s", "read=s", "output=s"));
Help()  if ($options{help});
Help(1) unless
    ($options{output} && $options{mapping} && $options{read} && $options{lines});

my $LeftHalfDnbNoMatches    = 1;
my $LeftHalfDnbMapOverflow  = 2;
my $RightHalfDnbNoMatches   = 4;
my $RightHalfDnbMapOverflow = 8;
my $LastDNBRecord = 1;
my $flag_side     = 2;
my $flag_strand   = 4;

my (@mapping, @reads);
{
    my $root = '/panfs/pan1/sra-test/golikov/loader/cg/YRI_trio.tiny/MAP';
    my $read = "$root/100.reads_GS10351-FS3-L01_001.tsv.bz2";
    $read = $options{read};
    push @reads, $read;
    my $map  = "$root/100.mapping_GS10351-FS3-L01_001.tsv.bz2";
    $map = $options{mapping};
    push @mapping, $map;
}

# (256 * 1024) 5244 5245
#              5244
    my $last = 1
#              2
; $last = $options{lines};
my $row = 0;
for (my $fidx = 0; $fidx <= $#reads; ++$fidx) {
    my $read = $reads[$fidx];
    my $map = $mapping[$fidx];
    die $read unless (-f $read);
    die $map  unless (-f $map);

    my $out_read = basename($read);
    $out_read =~ s/^.*(reads_.*)$/$1/;
    $out_read = "$options{output}/$out_read";

    my $out_map = basename($map);
    $out_map =~ s/^.*(mapping_.*)$/$1/;
    $out_map = "$options{output}/$out_map";

    if ($read =~ /\.bz2$/) {
             $out_read =~ s/\.bz2$//;
             my $cmd = "/usr/bin/bzcat $read";
             open CG_READ, "$cmd |" or die "cannot open $cmd";
    } else { open CG_READ, $read or die "cannot open $read"; }

    if ($map =~ /\.bz2$/) {
             $out_map =~ s/\.bz2$//;
             my $cmd = "/usr/bin/bzcat $map";
             open CG_MAP, "$cmd |" or die "cannot open $cmd";
    } else { open CG_MAP, $map or die "cannot open $map"; }

    open OUT_READ, ">$out_read" or die "cannot open $out_read";
    open OUT_MAP , ">$out_map"  or die "cannot open $out_map";

    for (; ; ) {
        last if (defined $last && $row >= $last);

        my ($flags_r, $reads, $scores);
        my $eof = 1;
        while (<CG_READ>) {
            print OUT_READ;
            $eof = 0;
            next if (/^#/);     # skip headers
            next if (/^$/);     # skip empty lines
            chomp;
            if (/^>flags\treads\tscores$/) {
                next;
            }
            ++$row;
            ($flags_r, $reads, $scores) = split /\t/;
            last;
        }
        last if ($eof);

        my $noLeftMap
            = $flags_r & ($LeftHalfDnbNoMatches  | $LeftHalfDnbMapOverflow);
        my $noRightMap
            = $flags_r & ($RightHalfDnbNoMatches | $RightHalfDnbMapOverflow);

        unless ($noLeftMap && $noRightMap) {
          while (<CG_MAP>) {
            print OUT_MAP;
            next if (/^#/);     # skip headers
            next if (/^$/);     # skip empty lines
            if (/^>/) {
                next;        
            }
            my ($flags_m, $chromosome,
                    $offsetInChr, $gap1, $gap2, $gap3, $weight, $mateRec)
                = split /\t/;
            my $side = ($flags_m & $flag_side) ? 1 : 0;
            last if ($flags_m & $LastDNBRecord);
          }
        }
    }

    close OUT_READ;
    close OUT_MAP;
    close CG_READ;
    close CG_MAP;
}

print STDERR "$row\n";
print `/opt/panfs/bin/pan_df -h $options{output}`;
die 'pan_df' if ($?);

################################################################################
# EOF #
