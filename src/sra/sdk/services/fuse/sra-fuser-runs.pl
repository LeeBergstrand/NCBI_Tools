#!/opt/perl-5.8.8/bin/perl -w

use strict;
use Data::Dumper;

my @srv = qw#/panfs/traces01.be-md.ncbi.nlm.nih.gov#;
my @vol = qw#sra0 sra1 sra2#;


my %runs;


foreach my $s (@srv) {
    foreach my $v (@vol) {
        print STDERR "Volume $s/$v\n";
        OneDir("$s/$v");
    }
}

#print STDERR Dumper(\%runs);

print "<?xml version='1.0' encoding='UTF-8'?>\n<FUSE>\n";

foreach my $typ (sort(keys(%runs))) {
    print "\t<Directory name='$typ'>\n";
    my %nums = %{$runs{$typ}};
    foreach my $num (sort(keys(%nums))) {
        print "\t\t<Directory name='$num'>\n";
        my %r = %{$nums{$num}};
        foreach my $run (sort(keys(%r))) {
            print "\t\t\t<SRA name='$run' path='" . $r{$run} . "' />\n";
        }
        print "\t\t</Directory>\n";
    }
    print "\t</Directory>\n";
}
print "</FUSE>\n";

sub OneDir
{
    my $d = shift;

    if( $d =~ /\/([ES]RR)$/ ) {
        print STDERR "$1 now ignored\n";
        return;
    }

    if( opendir(DIR, $d) ) {
        print STDERR "Dir: $d\n";
        my @rr = grep { $_ !~ /^\./ && -d "$d/$_" } readdir(DIR);
        closedir DIR;
        
        foreach my $r (@rr) {
#            if( $r =~ /([SDE]RR)(\d{6})$/ ) {
            if( $r =~ /(DRR)(\d{6})$/ ) {
                if( -f "$d/$r/lock" || -f "$d/$r/sealed" ) {
                    my $x = sprintf("%06u", int($2 / 1000) * 1000);
                    $runs{$1}{$x}{$r} = "$d/$r";
                } else {
                    print STDERR "$d/$r ignored: lock not found\n";
                }
            } else {
                OneDir("$d/$r");
            }
        }
        
    } else {
        warn "Can't opendir $d: $!";
    } 
}
