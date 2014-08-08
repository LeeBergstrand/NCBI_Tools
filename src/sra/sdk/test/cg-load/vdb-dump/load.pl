#!/usr/local/bin/perl -w
################################################################################

use strict;

use File::Basename 'basename';
use Getopt::Long "GetOptions";

sub Help {
    my ($exit) = @_;
    $exit = 0 unless (defined $exit);
    my $name = basename($0);
    print <<EndText;
Usage:
    $name --map <MAP input directory path containing files> --output <DB path>
        [ --bin <binary-directory-with cg-load> ] [ --asm <evidence-location> ]
        [ --norun ]
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options,
    "asm=s", "bin=s", "help", "map=s", "norun", "output=s"));
Help(0) if ($options{help});
Help(1) unless ($options{map} && $options{output});

my $kfg = GetKfg();
unless ($kfg) {
    die "Cannot detect CG files version. "
        . "Are you sure '$options{map}' contains source files?"
}

my $loader = '/panfs/traces01/trace_software/toolkit/centos64/bin/cg-load';
$loader = '/home/klymenka/cvs/internal.new/asm-trace/OUTDIR/bin64/cg-load';
$loader = "$options{bin}/cg-load" if ($options{bin});
die "$loader not found" unless (-e $loader);
my $cmd = "$loader -Q 0 -f -k $kfg -m $options{map} -o $options{output}";
$cmd .= " -a $options{asm}" if ($options{asm});

print STDERR "$cmd\n";
unless ($options{norun}) {
    print `$cmd 2>&1`;
    die $cmd if ($?);
}

sub GetKfg { return FindVersion($options{map}); }

sub FindVersion {
    my ($b36, $b37) =
        ('/panfs/pan1/sra-test/golikov/loader/cg/chr_build36/analysis.bam.cfg',
         '/panfs/traces01/compress/qa/yaschenk/CGNative/cg37_refseq.cfg');
    ($_) = @_;
    foreach (glob("$_/*")) {
        if (-d $_) {
            my $v = FindVersion($_);
            return $v if ($v);
        } else {
            my $v = GetVersion($_);
            if ($v) {
                return $b37 if ($v == 2);
                return $b36;
            }
        }
    }
}

sub GetVersion {
    ($_) = @_;
    my $cmd;
    if (/\.bz2$/) {
        $cmd = "/usr/bin/bzcat";
    } else {
        $cmd = '/bin/cat';
    }
    $cmd .= " $_ | /usr/bin/head -5";
    @_ = `$cmd`;
    foreach (@_) {
        return $1 if (/#FORMAT_VERSION\t(\d)\.\d$/);
    }
    return undef;
}

################################################################################
# EOF #
