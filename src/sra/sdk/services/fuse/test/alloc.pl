#!/opt/perl-5.8.8/bin/perl -w

use strict;
use Data::Dumper;

my ($q, $t) = (0,0);
my %x = ();

while(<>) {
    chomp;

    my $key;
    my $addr;
    my $old;
    my $bytes;
    
    if( $_ =~ /0=free\(([\dA-Fa-f]+)\)/ ) {
        $old = $1;
        foreach my $k (keys(%x)) {
            if( exists $x{$k}{chunks}{$old} ) {
                $x{$k}{mem} -= $x{$k}{chunks}{$old};
                $t -= $x{$k}{chunks}{$old};
                delete $x{$k}{chunks}{$old};
                last;
            }
        }
    } else {
        if( $_ =~ /\]\s+(\S+)\s+([\dA-Fa-f]+)=realloc\(([\dA-Fa-f]+),\s+(\d+)\)/ ) {
            $key = $1;
            $addr = $2;
            $old = $3;
            $bytes = $4;
        } elsif( $_ =~ /\]\s+(\S+)\s+([\dA-Fa-f]+)=[mc]alloc\((\d+)\)/ ) {
            $key = $1;
            $addr = $2;
            $bytes = $3;
        }
        if( $key ) {
            $x{$key} ||= ();
            $x{$key}{qty}++;
            $x{$key}{chunks} ||= ();
            if( $old ) {
                $x{$key}{mem} -= $x{$key}{chunks}{$old};
                $t -= $x{$key}{chunks}{$old};
                delete $x{$key}{chunks}{$old};
            }
            $x{$key}{mem} += $bytes;
            $x{$key}{chunks}{$addr} = $bytes;
            $q++; $t += $bytes;
        }
    }
}

foreach my $k (keys(%x)) {
    my $s = 0;
    foreach my $c (keys(%{$x{$k}{chunks}})) {
        $s += $x{$k}{chunks}{$c};
    }
    delete $x{$k}{chunks};
    $x{$k}{lost} = $x{$k}{mem} - $s;
    delete $x{$k}{lost} unless $x{$k}{lost};
}
print Dumper(\%x);
print "total $q " . int($t/1024/1024) . "M ($t)\n";
