#!/usr/local/bin/perl -w
use strict;
use Cwd "getcwd";
use File::Basename "basename";
use Getopt::Long "GetOptions";
use IO::Handle;
my $LINES = 0;
my %options;
Help() unless (GetOptions(\%options,
    "cg=s", "force", "in=s", "help", "ncbi=s", "out=s"));
Help() if ($options{help});
Help() unless ($options{cg} && $options{ncbi});
$options{in} = '.' unless ($options{in});
$options{out} = '.' unless ($options{out});
die "$options{in} is not a directory" unless (-d $options{in});
if (-e $options{out}) {
    die "$options{out} is not a directory" unless (-d $options{out});
    my @out = glob("$options{out}/*");
    if ($#out > 0) {
        Help("$options{out} is not empty\n\n") unless ($options{force});
        `rm $options{out}/*`;
        die if ($?);
    }
} else {
    mkdir $options{out} or die "cannot mkdir $options{out}";
}
CheckARGV();
STDOUT->autoflush(1);
my %FILES;
my ($NCBI_IN, $LINE_NCBI, @LINES_NCBI, $LINE_NO_NCBI_IN, %FLD_NCBI);
my ($CG_IN  , $LINE_CG  , @LINES_CG  , $LINE_NO_CG_IN  , %FLD_CG, @LINE_CG);
unless ($options{cg} =~ /\*$/) {
    RestoreNFilter();
} else {
    Help() unless ($options{ncbi} =~ /\*$/);
    my $ncbi = $options{ncbi};
    Help() unless ($ncbi =~ /^(.+)\*$/);
    my $ncbi_prefix = $1;
    my $cg   = $options{cg};
    die unless ($cg =~ /^(.+)\*$/);
    my $cg_prefix = $1;
    Help() unless ($ncbi && $cg);
    my $cwd = getcwd();
    chdir $options{in} or die "cannot cd $options{in}";
    my @ncbi = glob($ncbi);
    my @cg   = glob($cg);
    chdir $cwd or die "cannot cd $cwd";
    my (%ncbi, %cg);
    foreach (@ncbi) {
        die unless (/^($ncbi_prefix)(.*)$/);
        next if (/filtered$/ || /filtered\.nomatch$/);
        die if ($ncbi{$2});
        $ncbi{$2} = $_;
    }
    foreach (@cg) {
        die unless (/^($cg_prefix)(.*)$/);
        next if (/filtered$/ || /filtered\.nomatch$/);
        die if ($cg{$2});
        $cg{$2} = $_;
    }
    die if (keys %ncbi > keys %cg);
    die "cannot find any of '$options{cg}' '$options{ncbi}'"
        if (keys %ncbi == 0);
    foreach my $sfx (keys %cg) {
        my $header = $sfx =~ /^header$/;
        my $star = $sfx =~ /^STAR$/;
        if ($ncbi{$sfx}) {
            if ($header || $star) {
                print "$cg{$sfx}\t$ncbi{$sfx}\n" ;
                my $cmd = "cp -p $options{in}/$ncbi{$sfx} $options{out}";
                `$cmd`;
                die "cannot $cmd" if ($?);
            } else {
                $options{ncbi} = $ncbi{$sfx};
                $options{cg} = $cg{$sfx};
                delete $options{ncbi_in} if ($options{ncbi_in});
                delete $options{ncbi_out} if ($options{ncbi_out});
                delete $options{cg_in} if ($options{cg_in});
                delete $options{cg_out} if ($options{cg_out});
                RestoreNFilter();
            }
            delete $ncbi{$sfx};
        } else {
            print "HEADER " if ($header);
            print "STAR " if ($star);
            print "$cg{$sfx}\n";
        }
        if ($header || $star) {
            my $cmd = "cp -p $options{in}/$cg{$sfx} $options{out}";
            `$cmd`;
            die "cannot $cmd" if ($?);
        }
        delete $cg{$sfx};
    }
    die if (%ncbi);
}
sub RestoreNFilter {
    ProcessFileNames();
    print "$options{cg_in}\t$options{ncbi_in}";
    if ($options{cg_in} =~ /am\.\*$/ && $options{ncbi_in} =~ /sam\.\*$/) {
        print "\n";
        return;
    }
    my $cg_in = "$options{in}/$options{cg_in}";
    open($CG_IN, $cg_in) or die "cannot open '$cg_in'";
    $LINE_NO_CG_IN = 0;
    ($#LINES_CG, $#LINES_NCBI) = (-1, -1);
    my $ncbi_in = "$options{in}/$options{ncbi_in}";
    open($NCBI_IN, $ncbi_in) or die "cannot open '$ncbi_in'";
    $LINE_NO_NCBI_IN = 0;
    %FLD_NCBI = ();
    %FLD_CG = ();
    READ_NCBI();
    READ_CG();
MAIN: for (my $n = 1; ; ++$n) {
        last if ($LINES && $n > $LINES);
        unless ($LINE_NCBI) {
            last MAIN unless ($LINE_CG);
            print "\n";
            die;
            PrintCgNoMatch(@LINE_CG);
            PrintCgNoMatch(@LINE_CG) while (READ_CG());
            last MAIN;
        }
        die "$LINE_NCBI" unless ($LINE_CG);
        if ($FLD_NCBI{POS} == $FLD_CG{POS}) {
            if ($FLD_NCBI{MAPQ} == $FLD_CG{MAPQ}) {
                my $next_line_ncbi;
                while (1) {
                    $next_line_ncbi = READ_NCBI();
                    if ($next_line_ncbi) {
                        next if ($FLD_NCBI{POS} == $FLD_CG{POS}
                              && $FLD_NCBI{MAPQ} == $FLD_CG{MAPQ});
                        pop @LINES_NCBI;
                        die if ($#LINES_NCBI < 0);
                    }
                    $LINE_NCBI = $LINES_NCBI[0];
                    %FLD_NCBI = ParseLine($LINE_NCBI);
                    last;
                }
                my $next_line_cg;
                while (1) {
                    $next_line_cg = READ_CG();
                    if ($next_line_cg) {
                        next if ($FLD_NCBI{POS} == $FLD_CG{POS}
                              && $FLD_NCBI{MAPQ} == $FLD_CG{MAPQ});
                        pop @LINES_CG;
                        die if ($#LINES_CG < 0);
                    }
                    last;
                }
                if ($#LINES_CG == 0 && $#LINES_NCBI == 0) {
                    %FLD_CG = ParseLine($LINES_CG[0]);
                    @LINE_CG = GenerateRaw($LINES_CG[0], %FLD_CG);
                    PrintNCBIMatch($LINES_NCBI[0]);
                    PrintCgMatch(@LINE_CG);
                } else {
                    foreach $LINE_NCBI(@LINES_NCBI) {
                        %FLD_NCBI = ParseLine($LINE_NCBI);
                        my $found;
                        for (my $i_cg = 0; $i_cg <= $#LINES_CG; ++$i_cg) {
                            $LINE_CG = $LINES_CG[$i_cg];
                            next unless defined($LINE_CG);
                            %FLD_CG = ParseLine($LINE_CG);
                            @LINE_CG = GenerateRaw($LINE_CG, %FLD_CG);
                            my $SEQ = 9;
                            if ($LINE_CG[$SEQ] eq $FLD_NCBI{SEQ}) {
                                PrintNCBIMatch($LINE_NCBI);
                                PrintCgMatch(@LINE_CG);
                                delete $LINES_CG[$i_cg];
                                ++$found;
                                last;
                            }
                        }
                        unless ($found) {
                            print "\n";
                            die "$options{ncbi_in}, $options{cg_in} " .
                                "\@cg:$LINE_NO_CG_IN,ncbi:$LINE_NO_NCBI_IN";
                        }
                    }
                    foreach $LINE_CG (@LINES_CG) {
                        next unless defined($LINE_CG);
                        %FLD_CG = ParseLine($LINE_CG);
                        @LINE_CG = GenerateRaw($LINE_CG, %FLD_CG);
                        PrintCgNoMatch(@LINE_CG);
                    }
                }
                $#LINES_NCBI = -1;
                $LINE_NCBI = $next_line_ncbi;
                if ($LINE_NCBI) {
                    %FLD_NCBI = ParseLine($LINE_NCBI);
                    push @LINES_NCBI, $LINE_NCBI;
                }
                $#LINES_CG = -1;
                $LINE_CG = $next_line_cg;
                if ($LINE_CG) {
                    %FLD_CG = ParseLine($LINE_CG);
                    @LINE_CG = GenerateRaw($LINE_CG, %FLD_CG);
                    push @LINES_CG, $LINE_CG;
                }
                next MAIN;
            } elsif ($FLD_NCBI{MAPQ} < $FLD_CG{MAPQ}) {
                die "MAPQ(cg>ncbi)($FLD_NCBI{MAPQ} < $FLD_CG{MAPQ}): " .
                    "$LINE_NCBI";
            } else {
                PrintCgNoMatch(@LINE_CG);
                $#LINES_CG = -1;
                READ_CG();
                next MAIN;
            }
        } elsif ($FLD_CG{POS} < $FLD_NCBI{POS}) {
            print "\n";
            die;
            PrintCgNoMatch(@LINE_CG);
            while (READ_CG()) {
                next MAIN unless ($FLD_CG{POS} < $FLD_NCBI{POS});
                PrintCgNoMatch(@LINE_CG);
            }
        } else {
            print "\n";
            die "POS(cg<ncbi)\@cg:$LINE_NO_CG_IN,ncbi:$LINE_NO_NCBI_IN"
                . "($FLD_CG{POS} < $FLD_NCBI{POS}): $LINE_NCBI";
        }
    }
    print "\n";
    Close($CG_IN, $NCBI_IN);
}
sub READ_NCBI {
    $LINE_NCBI = <$NCBI_IN>;
    ++$LINE_NO_NCBI_IN;
    if ($LINE_NCBI) {
        %FLD_NCBI = ParseLine($LINE_NCBI);
        push @LINES_NCBI, $LINE_NCBI;
    }
    return $LINE_NCBI;
}
sub READ_CG {
    $LINE_CG = <$CG_IN>;
    ++$LINE_NO_CG_IN;
    if ($LINE_CG) {
        %FLD_CG = ParseLine($LINE_CG);
        @LINE_CG = GenerateRaw($LINE_CG, %FLD_CG);
        push @LINES_CG, $LINE_CG;
    }
    return $LINE_CG;
}
sub PrintCgMatch   { return PrintCg('cg_out'    , @_); }
sub PrintCgNoMatch { return PrintCg('cg_out_bad', @_); }
sub PrintNCBIMatch { return Print  ('ncbi_out'  , @_); }
sub PrintCg {
    my $file = shift;
    for (my $i = 0; $i <= $#_; ++$i) {
        Print($file, "\t") if ($i);
        Print($file, $_[$i]);
    }
    Print($file, "\n");
}
################################################################################
=begin COMMENT
SAM Format Specification 0.1.2-draft (20090820)
http://www.projet-plume.org/files/SAM1.pdf
SAM Format Specification 0.1.2-draft (20090820)
2. SAM Format Specification
2.2. Alignment Section
2.2.4. Format of optional fields
Note 6: Some bases from reads generated by Complete Genomics
may come from the same nucleotide.
The SEQ and QUAL fields always store the flattened sequence and quality
in that bases and qualities from the same nucleotide are collapsed to one.
The three optional tags GS/GQ/GC describes how to generate the raw read.
For example, given a raw read AAACGCGAAAA,
?CG? starting from 4th and 6th position come from the same oligonucleotide.
Suppose this read is mapped without gaps.
In SAM, the read alignment is stored as:
SEQ=AAACGAAAA, CIGAR=9M, GS:Z:CGCG, and GC:Z:3S2G4S,
where GS keeps the bases in the overlap
and GC says that to get the raw read sequence, we need to copy 3 bases from SEQ,
copy 2+2 bases from GS and then copy 4 bases from the SEQ field again.
#end COMMENT
=cut

sub GenerateRaw {
    my ($line, %fld) = @_;
    my ($SEQ, $QUAL) = ('', '');
    if ($fld{GC} || $fld{GS} || $fld{GQ}) {
        die "Bad alignment line '$line'"
            unless ($fld{SEQ} && $fld{QUAL}
                  && $fld{GC} && $fld{GS} && $fld{GQ});
        die "Bad alignment line:\nSEQ ='$fld{SEQ}'\nQUAL=$fld{QUAL}"
            unless (length($fld{SEQ}) == length($fld{QUAL}));
        die "Bad tags:\nGS='$fld{GS}'\nGQ='fld{GQ}'"
            unless (length($fld{GS}) == length($fld{GQ}));

        my ($n, $used_SEQ, $used_G) = (0, 0, 0);
        for (my $i = 0; $i < length($fld{GC}); ++$i) {
            my $c = substr($fld{GC}, $i, 1);
            if ($c =~ /\d/) {
                $n = $n * 10 + $c;
            } elsif ($c eq 'S') {
                $SEQ .= substr($fld{SEQ} , $used_SEQ, $n);
                $QUAL.= substr($fld{QUAL}, $used_SEQ, $n);
                $used_SEQ += $n;
                $n = 0;
            } elsif ($c eq 'G') {
                $SEQ .= substr($fld{GS}, $used_G, $n * 2);
                $QUAL.= substr($fld{GQ}, $used_G, $n * 2);
                $used_G += 2 * $n;
                $used_SEQ += $n;
                $n = 0;
            } else {
                die "'$fld{GC}': unexpected characted '$c' in GC";
            }
        }

        die "Bad GenerateRaw: $used_G: '$fld{GS}'; $used_SEQ: '$fld{SEQ}'"
            unless (length($fld{SEQ}) == $used_SEQ
                  && length($fld{GS}) == $used_G);
    } else {
        ($SEQ, $QUAL) = ($fld{SEQ}, $fld{QUAL});
    }

    my @line = ($fld{QNAME}, $fld{FLAG}, $fld{RNAME}, $fld{POS}, $fld{MAPQ},
        $fld{CIGAR}, $fld{RNEXT}, $fld{PNEXT}, $fld{TLEN}, $SEQ, $QUAL);
    push @line, "RG:Z:$fld{RG}" if ($fld{RG});

    return @line;
}
sub ParseLine {
    ($_) = @_;
    my %fld;
    my @optional;
    ($fld{QNAME}, $fld{FLAG}, $fld{RNAME}, $fld{POS}, $fld{MAPQ}, $fld{CIGAR},
        $fld{RNEXT}, $fld{PNEXT}, $fld{TLEN}, $fld{SEQ}, $fld{QUAL}, @optional)
      = split /\t/;
    foreach (@optional) {
        die "Bad optinal alignment line field: '$_'"
            unless (/^([A-Za-z][A-Za-z0-9]):([AifZHB]):(.+)/);
        my ($TAG, $TYPE, $VALUE) = ($1, $2, $3);
        if ($TAG eq 'RG' || $TAG eq 'GS' || $TAG eq 'GC' || $TAG eq 'GQ') {
            die "'$_': Unexpected type '$TYPE' of optional field tag '$TAG'"
                unless ($TYPE eq 'Z');
            $fld{$TAG} = $VALUE;
        } elsif ($TAG eq 'NM') {
            die "'$_': Unexpected type '$TYPE' of optional field tag '$TAG'"
                unless ($TYPE eq 'i');
            $fld{$TAG} = $VALUE;
        } else {
            chomp;
            die "'$_': Unexpected optinal alignment line field tag '$TAG'";
        }
    }
    return %fld;
}
sub Close {
    foreach (@_) {
        close $_;
    }
    foreach (keys %FILES) {
        close $FILES{$_};
        delete $FILES{$_};
    }
}
sub Print {
    my ($key, $line) = @_;
    die $key unless ($options{$key});
    my $name = $options{$key};
    unless ($FILES{$key}) {
        print "\t$name";
        $name = "$options{out}/$name";
        open(my $fh, ">$name") or die "cannot open $name";
        $FILES{$key} = $fh;
    }
    print { $FILES{$key} } $line or die "cannot print to $name";
}
sub ProcessFileNames {
    die "bad options" if ($options{ncbi_in} || $options{ncbi_out}
        || $options{cg_in} || $options{cg_out});
    die "bad in options" unless ($options{ncbi} && $options{cg});
    $options{ncbi_in} = $options{ncbi};
    $options{cg_in} = $options{cg};
    $options{ncbi_out} = "$options{ncbi}.filtered";
    $options{cg_out}   =  "$options{cg}.filtered";
    $options{ncbi_out_bad} = "$options{ncbi}.filtered.nomatch";
    $options{cg_out_bad}   =  "$options{cg}.filtered.nomatch";
    my $found;
    foreach ($options{ncbi_in}, $options{cg_in}) {
        my $name = "$options{in}/$_";
        unless (-e $name) {
            ++$found;
            print "$name: not found\n";
        }
    }
    if ($found) {
        Help("\n");
    }
    foreach ($options{ncbi_out}, $options{cg_out},
        $options{ncbi_out_bad}, $options{cg_out_bad})
    {
        my $name = "$options{out}/$_";
        if (-e $name) {
            unless ($options{force}) {
                ++$found;
                print "$name exists\n";
            } else {
                unlink or die "cannot unlink '$name'";
            }
        }
    }
    if ($found) {
        Help("\n");
    }
}
sub CheckARGV {
    Help() unless ($options{ncbi} && $options{cg});
}
sub Help {
    my $name = basename($0);
    print STDERR @_ if (@_);
    print STDERR <<EndText;
Usage:
    $name --ncbi <sam-dump-file> --cg <map2sam-dump-file> [--force]
          [--in <input-dir>] [--out <output-dir>]
        where
            <sam-dump-file>     is the result of "sam-dump SRR"
            <map2sam-dump-file> is the result of cgatools map2sam
        Use --force to overwrite existing output files.
        When <sam-dump-file> and <map2sam-dump-file> end with a *:
            each of <sam-dump-file>-s is matched
            with corresponding <map2sam-dump-file>. See example below.
        Input files: <sam-dump-file>
                     <map2sam-dump-file>
        Output files:
            <sam-dump-file>.filtered :
                <sam-dump-file> filtered and reordered
                that its lines match to <map2sam-dump-file>.filtered
            <map2sam-dump-file>.filtered : <map2sam-dump-file> filtered
                that its lines match to <sam-dump-file>.filtered
            <sam-dump-file>.filtered.nomatch :
                contains the part of lines of <sam-dump-file>
                that do not match to any line of <map2sam-dump-file>
            <map2sam-dump-file>.filtered.nomatch :
                contains the part of lines of <map2sam-dump-file>
                that do not match to any line of <sam-dump-file>

    $name -h
    $name --help
                    print this help

Examples:

to convert a pair of files:
\$ $name --ncbi SRZ-tiny.sam.chr17 --cg 100.sam.chr17

to convert a list of files:
\$ $name --ncbi 'SRZ-tiny.sam.*' --cg '100.sam.*'
The previous command will convert
all pairs of SRZ-tiny.sam.* and 100.sam.* matched by suffix
except the ones having suffix '.filtered' or '.filtered.nomatch'
EndText

    exit 1;
}
