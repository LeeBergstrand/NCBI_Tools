use strict;

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

1
