#!/opt/perl-5.8.8/bin/perl -w

use strict;
use Data::Dumper;

print "Start " . $ARGV[0] . "\n";
DoDir($ARGV[0]);

sub DoDir {
    my $d = shift;

    if( opendir(DIR, $d) ) {
        print "Dir: $d\n";
        my @rr = grep { $_ !~ /^\.+$/ } readdir(DIR);
        closedir DIR;
        print Dumper(\@rr);
        
        foreach my $r (@rr) {
            my $o = "$d/$r";
            if( -d $o ) {
                DoDir($o);
            } elsif( (-l $o || -f $o) && ! -z $o ) {
                print "Try read $o\n";
                ReadFile($o);
            } else {
                print "Skipped $o\n"
            }
        }
        
    } else {
        warn "Can't opendir $d: $!";
    }
}

sub ReadFile {
    my $f = shift;
    if( open(C, $f) ) {
        my $sz = 1024 * 200; #-s $f;
        seek C, ($sz / 2) - ($sz / 6), 0;
        if( !read(C, $sz, $sz / 3) ) {
            warn "Can't read $f: $!";
        }
        close C;
    } else {
        warn "Can't open $f: $!";
    }
}
