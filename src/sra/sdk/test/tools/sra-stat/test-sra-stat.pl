#!/opt/perl-5.8.8/bin/perl -w

################################################################################

use strict;
my $SKIP_FILE
    = '/panfs/pan1/sra-test/jira/VDB-535/runs-with-duplicated-SPOT_COUNT.list';
my %TO_SKIP = (SRR187776 => 'TABLE/BASE_COUNT < sum(SPOT_GROUP/BASE_COUNT)');

my $TRACES01 = '/panfs/traces01';
$_ = `hostname`;
my $sandbox;

if (/^wgasra0\d$/) {
    ++$sandbox;
    $TRACES01 = '/panfs/sandtraces01';
}

#my $DEV = "$ENV{HOME}/cvs/internal.new/asm-trace/OUTDIR/bin64";
my $PROD = "$TRACES01/trace_software/toolkit/centos64/bin";

my $OLD = "$PROD/sra-stat.2.0.5";
my $NEW =
 "/panfs/pan1/sra-test/asm-trace-new/OUTDIR/centos/pub/gcc/x86_64/bin/sra-stat";
$NEW = "$PROD/sra-stat";

Element::setOldSraStat($OLD);
Element::setNewSraStat($NEW);

use FindBin qw($Bin);
use lib $Bin;

use Element;
use XML::LibXML;

STDOUT->autoflush(1);

my $rand_only;
++$rand_only if ($#ARGV == 0 && $ARGV[0] eq 'rand');

if ($#ARGV >= 0 && ! $rand_only) {
    test($_) foreach @ARGV;
    exit 0;
}

#VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
unless ($rand_only || $sandbox) {
test($_) foreach (
 "DRR001178"# RD_FILTER column size is 1 but it is expected to be 2
,"ERR029139"# RD_FILTER column size is 2 but it is expected to be 3
,"ERR029904"# RD_FILTER column size is 2 but it is expected to be 4
,"SRR000061"# no-spot-groups example; spot-group in meta but not in table
,"SRR002609"# had to adjust stdev test precision in sra-stat.c (diff > 1e-10)
,"SRR006061"# spot-groups example
,"SRR094508"# a run with a single non-'default' member
,"SRR101385"# had to adjust stdev test precision in sra-stat.c (diff > 1e-9)
,"SRR116384"# a run with a single 'default' member

,"SRR125838"# VDB-535: SRATable:{spot_count = 2 * max_spot_id} RELOADED
,"SRR126415"# [doubled] Mismatch between calculated and recorded statistics
,"SRR166850"# with filtered stats
,"SRR185827"# STATS meta has no SPOT_GROUP submode, just TABLE
,"SRR187776"# VDB-534: STATS meta: TABLE/BASE_COUNT < sum(SPOT_GROUP/BASE_COUNT)
,"SRR342033"#VDB-538:double free;kdbmeta:"STATS/SPOT_GROUP/0/1_GCTAT".   reloadd
,"SRR342037"#VDB-538:double free;kdbmeta:"STATS/SPOT_GROUP/0/1_GCTAT".nt-reloadd
,"SRR345244"# old sra-stat fails

,"SRR346170"#a run with an empty default spot-group(all values are 0)in run meta
#"SRR346170"# member_name starts with a number: had to ajust test-sra-stat.pl

,"SRR353420"# table
,"SRR353922"# non-XML-complien library/sample (has BAM-header)
,"SRR362221"# has bad counts
,"SRR385888"# with align-info: should be local
,"SRR390443"# with bam-attributes: had to fix Element.pm: library looks like number="4_p..."
,"SRR395129"# with remote align-info
); # SRR391498 @ dbgap : has cmp_base_count : fixed Element.pm
}
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

{ #VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
    my %dir;
    foreach (`ls -d $TRACES01/*ra[0-9]/*R/*`) {
        chomp;
        $dir{$_} = {};
    }

    my $i = 0;

# 0 <= int(rand(2)) < 2
    while (keys %dir) {
        my $nD = keys %dir;
        my $r = int(rand($nD));
        my $dir = (keys %dir)[$r];
        my $list = $dir{$dir};

        unless (keys %$list) {
            foreach (`ls $dir`) {
                chomp;
                ++$list->{$_};
            }
        }

        my $nL = keys %$list;
        if ($nL) {
            $r = int(rand($nL));
            my $run = (keys %$list)[$r];

            print ++$i;
            print " | $nD | $dir | $nL\n";

            if (rand > 0.5) { test($run); }
            else     { test("$dir/$run"); }

            delete $list->{$run};
        } else { print "$dir $nD / $nL\n"; }
        delete($dir{$dir}) unless (keys %$list);
    }
} #^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

# # member quick xml
# 0
# 1              -x
# 2        -q
# 3        -q    -x
# 4 off
# 5 off          -x
# 6 off    -q
# 7 off    -q    -x

sub GetAcc {
    my ($path) = @_;
    my $acc = $path;
    $acc = $1 if ($path =~ m|^/.+/(.+)$|);
    return $acc;
}

sub IsDoubled {
    my ($acc) = @_;
    `/bin/grep -q $acc $SKIP_FILE`;
    return $? == 0;
}

################################################################################
sub test {
    my ($path) = @_;

    if ($path =~ /SRR\d{6}\.sra0[1-4]\.\d+$/) {
        print "$path skipped as beeing loaded\n\n";
        return 1;
    }

    my $acc = GetAcc($path);

    my $reason = $TO_SKIP{$acc};
    if ($reason) {
      print "$path skipped ($reason))\n\n";
      return 1;
    }
    if (IsDoubled($acc)) {
        print "$path skipped (spot_count = 2 * max_spot_id)\n\n";
        return 1;
    }

    my $full = $path;
    unless ($path =~ /^\//) {
        $full = `$PROD/srapath $path`;
        die $path if ($?);
        chomp $full;
    }
    my $table = -e "$full/tbl";
    if (-f $full) {
        my $cmd = "$PROD/kar -t $full tbl | wc -l";
        my $tmp = `$cmd`;
        die $cmd if ($?);
        chomp $tmp;
        ++$table if ($tmp);
    }

    print `date`;
    my @run;

TEST: for (my $i = 0; $i < 8; ++$i) {
#       $i = 2 if ($i == 1); last if ($i == 3);# $i = 3 if ($i == 2);
# skip quick $i = 4 if ($i == 2); last if ($i == 6);

        my $sra_stat = $NEW;
        my ($run, $total) = sra_stat($sra_stat, $path, $i, 'test-stdev');
        push @run, $run if ($run);
        push @run, $total if ($total);

        if (1 && $i < 2 && ! $table) {
            foreach ($path) {
                next TEST if(
                    /DRR001178$/ || # old app reports bad counts incorrectly
             /SRR015505$/ || # base count is reported differently by the old app
                    # old app fails on some runs:
                 /ERR016495$/ || /SRR050362$/ || /SRR343094$/ || /SRR345649$/ ||
                 /SRR343078$/ || /SRR345244$/ || /SRR346171$/
                );
            }

            my $sra_stat = $OLD;
            my ($run, $total) = sra_stat($sra_stat, $path, $i);
            push @run, $run if ($run);
            push @run, $total if ($total);
        }
    }

    for (my $t = 0; $t < $#run; ++$t) {
        for (my $t2 = $t + 1; $t2 <= $#run; ++$t2) {
            if (0) {
                print "run[$t](";
                print $run[$t ]->toString();
                print ")\trun[$t2](";
                print $run[$t2]->toString() . ")\n";
            }

            $run[$t]->equals($run[$t2], 1);
        }
    }

    print "\n";
}

sub AlignInfo {
    my ($runPath, $Run) = @_;
    my @nAlignInfo = $Run->findnodes('AlignInfo');
    die unless ($#nAlignInfo == 0);
    my @nRef = $nAlignInfo[0]->findnodes('Ref');
    print("\tAlignInfo\n");
    if ($#nRef == -1) {
        my $cmd = "$PROD/kdbmeta $runPath | /usr/bin/head -1";
        my $meta = `$cmd`;
        die $cmd if ($?);
        return if ($meta =~ /^<schema name="NCBI:SRA:PacBio:smrt:db#1">\n$/);
    } else {
        foreach my $Ref (@nRef) {
            my @attributes = $Ref->attributes();
            my ($local, $path);
            foreach (@attributes) {
                my $n = $_->nodeName;
                if ($n =~ /^circular$/) {
                    die "$n $runPath AlignInfo"
                        unless ($_->value eq 'false' || $_->value eq 'true');
                } elsif ($n =~ /^local$/) {
                    $local = $_->value;
                } elsif ($n =~ /^name$/) {
                    die "$n $runPath AlignInfo" unless ($_->value);
                } elsif ($n =~ /^seqId$/) {
                    die "$n $runPath AlignInfo" unless ($_->value);
                } elsif ($n =~ /^path$/) {
                    $path = $_->value;
                } else {
                    die "$n $runPath AlignInfo";
                }
            }
            die "$runPath AlignInfo" unless ($local);
            if ($local eq 'local') {
                die "$runPath AlignInfo local" if ($path);
            } elsif ($local eq 'remote') {
                die "$runPath AlignInfo local" unless ($path || ! -e $path);
            } else {
                die "$runPath AlignInfo local";
            }
        }
        return 1;
    }
    die "$runPath AlignInfo";
}

sub sra_stat {
    my ($sra_stat, $path, $i, $test_stdev) = @_;
    my $matrix = $i;

    my $xml    = $matrix % 2; $matrix >>= 1;
    my $quick  = $matrix % 2; $matrix >>= 1;
    my $member = $matrix % 2; $matrix >>= 1;
    die if ($matrix);

    my $cmd = "$sra_stat $path";
    $cmd .= " --member-stats off" if ($member); # NB members are OFF if($member)
    $cmd .= " -q" if ($quick);
    $cmd .= " -x" if ($xml);
    $cmd .= ' -t' if ($test_stdev && $xml && ! $quick);
    print "$i $cmd\n";
    my @in = `$cmd 2>&1`;
    die $cmd if ($?);
    my $in;
    foreach (@in) {
        if (/ warn: Statistics metadata not found/) {
            $quick = 0;
            next;
        }
        if (/ warn: Mismatch between calculated and recorded statistics/) {
            die "$path: $_";
        }
        next
           if (/ warn: RD_FILTER column size is 1 but it is expected to be 2$/);

       # all values are SRA_READ_FILTER_PASS, SRA_READ_FILTER_PASS
        next
           if (/ warn: RD_FILTER column size is 2 but it is expected to be 3$/
            && ($path =~ /ERR02910[45]$/ || $path =~ /ERR0291[34][0789]$/ ||
                $path =~ /ERR02941[789]$/ ));
        next
           if (/ warn: RD_FILTER column size is 2 but it is expected to be 4$/
            && ($path =~ /ERR02990[4-78]$/ ));

        $in .= $_;
    }

    my ($run, @nMember);
    if ($in =~ /</) {
        my $parser = XML::LibXML->new();
        my $root = $parser->parse_string($in)->getDocumentElement;
        my @nRun = $root->findnodes('/Run');
        die unless ($#nRun == 0);
        $run = new Element($cmd, 'Run', $i, $quick, $xml, $member, $nRun[0]);
        @nMember = $nRun[0]->findnodes('Member');
        if ($in =~ /AlignInfo/) {
            AlignInfo($path, $nRun[0]);
        }
    }
    elsif ($member) {
        $run = new Element($cmd, 'sra-stat', $i, $quick, $xml, $member, $in);
    } else { @nMember = split(/\n/, $in); }
    my $total = new Element($cmd, 'total', $i, $quick, $xml, $member);
    if (@nMember) {
        die if ($member);
        foreach (@nMember) {
            my $m = new Element($cmd, 'Member', $i, $quick, $xml, $member, $_);
            $total->add($m);
            $run->addMember($m) if ($run);
        }
        $total->equals($run) if ($run);
        return ($run, $total);
    }
    return $run;
}

################################################################################
