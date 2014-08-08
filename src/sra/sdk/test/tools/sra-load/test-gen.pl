#!/opt/perl-5.8.8/bin/perl -w

use strict;
use Data::Dumper;
use POSIX qw(strftime);

#IUPAC plus '.-' as 'N'
my $BASES_ALLOWED = "ACGT"; #URYSWKMBDHVN.-";

#CYCLE_COORD is treated in code same way as BASE_COORD so we are not testing it
my @types = qw/BASE_COORD RELATIVE_ORDER EXPECTED_BASECALL_TABLE /; #EXPECTED_BASECALL /;

# setup reproducable session using unique id
my $seed = $ARGV[0] ? $ARGV[0] : (time ^ ($$ + ($$ << 15)));
srand($seed);

my $g_descr_qty = 0;
my $g_max_spot_len ;

print `rm *.spots_*`;

my ($reads_min, $reads_max) = (3, 1);
for(my $i = $reads_min; $i >= $reads_max; $i--) {
    my @spot = ();
    NextRead($i, \@spot);
}
exit(0);

sub NextRead
{
    my ($lvl, $spot) = @_;
    
    if($lvl <= 0) {
        ProcSpot($spot);
    } else {
        for(my $i = 0; $i < scalar(@types); $i++ ) {
            push @$spot, $types[$i];
            NextRead($lvl - 1, $spot);
            pop @$spot;
        }
    }
}

sub ProcSpot
{
    my $spot = shift;
    my @table = ();

    my $line = sprintf("%3u. ", ++$g_descr_qty);

    for(my $t = 0; $t < scalar(@$spot); $t++) {
        my $s = @$spot[$t];
        my %h;
        $table[$t] = \%h;
        $line .= sprintf("%s%23s", $t ? ', ' : '', $s);
        $h{type} = $s;
        $h{ebc_qty} = $s eq 'EXPECTED_BASECALL_TABLE' ? (int(rand(20)) + 1) : 0;
        if( $t > 0 ) {
            if( $s eq 'RELATIVE_ORDER' and @$spot[$t - 1] !~ /^EXPECTED_BASECALL/ ) {
                $line = "bad-> $line" unless $line =~ /^bad/;
            }
        } elsif( scalar(@$spot) == 1 and $s eq 'RELATIVE_ORDER') {
            $line = "bad-> $line" unless $line =~ /^bad/;
        }
    }
    if($line =~ /^bad/ ) {
        --$g_descr_qty;
        return;
    }
    print "$line\n";
    
    my $descr = '';
    for(my $t = 0; $t < scalar(@table); $t++) {
        $descr .= ', ' unless $t == 0;
        $descr .= $table[$t]->{type};
    }

    open E, ">$g_descr_qty-exp.spots_xml" || die "Can't write Experiment: $?";
    print E '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
    print E "<EXPERIMENT alias='test_spot_splitter' center_name='NCBI'>\n\t<TITLE>Test case $g_descr_qty, on seed $seed</TITLE>\n\t<STUDY_REF refname='test_spot_splitter' refcenter='NCBI'/>\n";
    print E "\t<DESIGN>\n\t<DESIGN_DESCRIPTION/>\n\t<SAMPLE_DESCRIPTOR refname='test_spot_splitter' refcenter='NCBI'>\n";

    # calc EXPECTED..TABLEs intersection size
    my $grp_qty = 0;
    foreach my $read (@table) {
        if( $read->{ebc_qty} > 0 ) {
            $grp_qty = 1 unless $grp_qty > 0;
            $grp_qty *= $read->{ebc_qty};
        }
    }
    if( $grp_qty ) {
        my %pool = ();
        for(my $t = 0; $t < scalar(@table); $t++) {
            next unless $table[$t]->{ebc_qty} > 0;
            my $i = 0;
            for(my $g = 0; $g < $grp_qty; $g++) {
                push @{$pool{$g}}, [$t, $i];;
                $i = $i < $table[$t]->{ebc_qty} - 1 ? ($i + 1) : 0;
            }
        }
        print E "<POOL debug_qty='$grp_qty'>\n";
        for(my $g = 0; $g < $grp_qty; $g++) {
            print E "\t<MEMBER member_name='Group_$g'>\n";
            foreach my $m (@{$pool{$g}}) {
                print E "\t\t<READ_LABEL read_group_tag='tag_$m->[0]_$m->[1]'>Read_$m->[0]</READ_LABEL>\n";
            }
            print E "\t</MEMBER>\n";
        }
        print E "</POOL>\n";

    }
    print E "\t</SAMPLE_DESCRIPTOR>\n\t<LIBRARY_DESCRIPTOR />\n\t<SPOT_DESCRIPTOR>\n<SPOT_DECODE_SPEC>\n";
    
    $g_max_spot_len = 0;
    for(my $t = 0; $t < scalar(@table); $t++) {
        no strict "refs";
        my $r = $table[$t]->{type}($t, $table[$t]);
        $g_max_spot_len += $table[$t]->{max_len};
        use strict;
        print E $r;
    }
    print E "<!--\n" . Dumper(\@table) . " -->\n";

    print E "</SPOT_DECODE_SPEC>\n\t</SPOT_DESCRIPTOR>\n\t</DESIGN>\n";

    print E "\t<PLATFORM>\n";
    print E "\t  <ILLUMINA>\n";
    print E "\t    <INSTRUMENT_MODEL>Illumina Genome Analyzer II</INSTRUMENT_MODEL>\n";
    print E "\t    <CYCLE_COUNT>144</CYCLE_COUNT>\n";
    print E "\t    <SEQUENCE_LENGTH>144</SEQUENCE_LENGTH>\n";
    print E "\t  </ILLUMINA>\n";
    print E "\t</PLATFORM>\n";
    print E "\t<PROCESSING>\n";
    print E "\t    <BASE_CALLS>\n";
    print E "\t        <SEQUENCE_SPACE>Base Space</SEQUENCE_SPACE>\n";
    print E "\t        <BASE_CALLER>OLB-1.6.0</BASE_CALLER>\n";
    print E "\t    </BASE_CALLS>\n";
    print E "\t    <QUALITY_SCORES>\n";
    print E "\t        <QUALITY_SCORER>OLB-1.6.0</QUALITY_SCORER>\n";
    print E "\t        <NUMBER_OF_LEVELS>80</NUMBER_OF_LEVELS>\n";
    print E "\t        <MULTIPLIER>1</MULTIPLIER>\n";
    print E "\t    </QUALITY_SCORES>\n";
    print E "\t</PROCESSING>\n";
    
    print E "</EXPERIMENT>\n";
    close E;
    open R, ">$g_descr_qty-run.spots_xml" || die "Can't write Run: $?";
    print R '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
    print R "<RUN alias='test_spot_splitter' center_name='NCBI' run_date='" . strftime("%Y-%m-%dT%H:%M:%S.0Z", localtime) . "'  run_center='NCBI' instrument_name='NULL'>\n";
    print R "\t<EXPERIMENT_REF refname='test_spot_splitter' refcenter='NCBI' />\n";
    print R "\t<DATA_BLOCK serial='1' name='SINGLE_BLOCK' region='0'>\n\t\t<FILES>\n\t\t\t<FILE filename='$g_descr_qty-Run.spots_in' filetype='spots' />\n";
    print R "\t\t</FILES>\n\t</DATA_BLOCK>\n\t<RUN_ATTRIBUTES/>\n</RUN>\n";
    close R;
    GenSpotData(\@table, $g_max_spot_len);
}

sub GenSpotData
{
    my ($tbl, $max_spot_len) = @_;

    open S1, ">$g_descr_qty-dat.spots_in" || die "Can't write Spot data: $?";
    open S2, ">$g_descr_qty-dat.spots_split" || die "Can't write Spot split data: $?";
    
    my $cases = 0;
    my $tmax = scalar(@$tbl) - 1;
    for(my $t = 0; $t <= $tmax; $t++) {
        my $sample_qty;
        my $r = @$tbl[$t];

        if( $r->{ebc_qty} ) {
            $sample_qty = $r->{ebc_qty};
            for(my $i = 0; $i < $sample_qty; $i++) {
                $r->{sample}[$i] = 'SSS';
            }
        } else {
            $sample_qty = int(rand(11)) + 5;
            for(my $i = 0; $i < $sample_qty; $i++) {
                my $l = $r->{len};
                if( ($t < $tmax && @$tbl[$t + 1]->{fixed_start} < 0) || $t == $tmax ) {
                    $l = int(rand($r->{max_len})) + $r->{max_len}; # not terminated on right
                }
                $r->{sample}[$i] = RandomSeq($r->{len}, $l);
            }
        }
        $cases = $sample_qty if $cases < $sample_qty;
        $r->{samples} = $sample_qty;
    }
    print S1 "$cases\n";
    print S2 "$cases\n";

    my @lines;
    foreach my $r (@$tbl) {
        my $rq = $r->{samples};
        for(my ($g, $k) = (0, 0); $g < $cases; $g++, $k++) {
            if($k >= $rq ) {
                $k = 0;
            }
            $lines[$g] ||= '';
            $lines[$g] = join(' ', $lines[$g], $r->{sample}[$k]);
        }
    }
    
    foreach my $l (@lines) {
        print S2 substr($l, 1) . "\n";
        $l =~ s/ //g;
        print S1 "$l\n";
    }
    close S1;
    close S2;
}

sub BASE_COORD
{
    my ($id, $o) = @_;
    $o->{start} = $g_max_spot_len;
    $o->{fixed_start} = $g_max_spot_len;
    $o->{len} = int(rand(15)) + 5;
    $o->{max_len} = int($o->{len} * 1.1);
    return ReadOpen($id) . "\t<BASE_COORD>" . ($o->{start} + 1). "</BASE_COORD>\n" . ReadClose($id);
}

sub RELATIVE_ORDER
{
    my ($id, $o) = @_;
    my $a = $id == 0 ? "precedes_read_index='1'" : "follows_read_index='" . ($id - 1) . "'";
    $o->{start} = $g_max_spot_len;
    $o->{fixed_start} = -1;
    $o->{len} = int(rand(20)) + 10;
    $o->{max_len} = int($o->{len} * 1.2);
    return ReadOpen($id) . "\t<RELATIVE_ORDER $a />\n" . ReadClose($id);
}

sub EXPECTED_BASECALL
{
    my ($id, $o) = @_;
    my $seq = RandomSeq(4, 18);
    $o->{ebc_qty} = 1;
    $o->{start} = $g_max_spot_len;
    $o->{fixed_start} = rand() < .5 ? $o->{start} : -1;
    $o->{len} = length($seq);
    $o->{max_len} = $o->{len};
    $o->{bc}[0] = {'bc', $seq, 'len', length($seq), 'min', length($seq), 'max', 0, 'edge', 'full'};

    return ReadOpen($id) . "\t<EXPECTED_BASECALL"
       . (rand() < .5 ? ' default_length="' . $o->{len}  . '"' : '')
       . ($o->{fixed_start} >= 0 ? ' base_coord="' . ($o->{fixed_start} + 1) . '"' : '')
       . ">" . $seq . "</EXPECTED_BASECALL>\n" . ReadClose($id);
}

sub EXPECTED_BASECALL_TABLE
{
    my ($id, $o) = @_;
    my $mid_seq_len = int(rand(36)) + 6;
    my $max_mismatch = int($mid_seq_len * 0.1); # 10% errors

    my $edge = rand();
    $edge = $edge < 0.1 ? "end" : $edge < 0.3 ? "start" : "full";
    
    $o->{bc} = [];
    $o->{len} = 0;
    for(my $i = 0; $i < $o->{ebc_qty}; $i++) {
        my $seq = RandomSeq($mid_seq_len - 2, $mid_seq_len + 2);
        $o->{len} = length($seq) if $o->{len} < length($seq);
        $o->{bc}[$i] = {'bc', $seq, 'len', length($seq), 'min', length($seq) - $max_mismatch, 'max', $max_mismatch, 'edge', $edge};
    }

    $o->{start} = $g_max_spot_len;
    $o->{fixed_start} = rand() < .5 ? $o->{start} : -1;
    $o->{max_len} = int($o->{len} * 1.05); # 5% inserts

    my $res = ReadOpen($id) . "\t<EXPECTED_BASECALL_TABLE"
       . (rand() < .5 ? ' default_length="' . $o->{max_len}  . '"' : '')
       . ($o->{fixed_start} >= 0 ? ' base_coord="' . ($o->{fixed_start} + 1) . '"' : '')
       . ">\n";
    for(my $i = 0; $i < $o->{ebc_qty}; $i++) {
        $res .= "\t\t<BASECALL read_group_tag='tag_${id}_${i}'"
        . " min_match='" . $o->{bc}[$i]->{min} . "'"
        . " max_mismatch='" . $o->{bc}[$i]->{max} . "'"
        . " match_edge='" . $o->{bc}[$i]->{edge} . "'"
        . ">" . $o->{bc}[$i]->{bc} . "</BASECALL>\n";
    }
    $res .= "\t</EXPECTED_BASECALL_TABLE>\n" . ReadClose($id);
    return $res;
    
}

sub ReadOpen
{
    my $id = shift;
    my @tt = ("Technical Read", "Application Read");
    my @rt = ("Forward", "Reverse", "Adapter", "Primer", "Linker", "BarCode", "Other");
    
    return "<READ_SPEC>\n\t<READ_INDEX>$id</READ_INDEX>\n" .
           "\t<READ_LABEL>Read_$id</READ_LABEL>\n" .
           "\t<READ_CLASS>" . $tt[int(rand(scalar(@tt)))] . "</READ_CLASS>\n" .
           "\t<READ_TYPE>" . $rt[int(rand(scalar(@rt)))] . "</READ_TYPE>\n";
}

sub ReadClose
{
    return "</READ_SPEC>\n";
}

sub RandomSeq
{
    my ($min_len, $max_len) = @_;
    my $len = int(rand($max_len - $min_len + 1)) + $min_len;
    my $bases = '';

    for(my $i = 0; $i < $len; $i++) {
        $bases .= substr($BASES_ALLOWED, int(rand(length($BASES_ALLOWED))), 1);
    }
    return $bases;
}
