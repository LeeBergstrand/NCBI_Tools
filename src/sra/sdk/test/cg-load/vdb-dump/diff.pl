#!/usr/local/bin/perl -w
################################################################################

use strict;

use File::Basename 'basename';
use Getopt::Long "GetOptions";
use IO::Handle; # autoflush
use POSIX ':sys_wait_h'; # WNOHANG

my $appname = basename($0);

sub Help {
    my ($exit) = @_;
    $exit = 0 unless (defined $exit);
    my $stream = *STDOUT;
    $stream = *STDERR if ($exit);
    print $stream <<EndText;
Usage:
    $appname -1 <files-prefix-1> -2 <files-prefix-2>
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options,
    "help", "1=s", "2=s"));
Help(0) if ($options{help});
Help(1) unless ($options{1} && $options{2});

my %list = Pair($options{1}, $options{2});

STDERR->autoflush(1);

my $CHILDREN_MAX;
my %child;
my $error = 0;
foreach (keys %list) {
    while (1) {
        {
            my $kid = waitpid(-1, WNOHANG);
            if ($kid > 0) {
                my $status = $?;
                my $date = `/bin/date`;
                die 'date' if ($?);
                chomp $date;
                if ($status) {
                    $error = 1;
                    print STDERR "$date\tFAILED $child{$_}: $!\n";
                } else { print STDERR "$date\tdone with $child{$kid}\n"; }
                delete $child{$kid};
                last if ($error);
            }
        }
        if (isIdle()) {
            if (!defined(my $pid = fork)) {
                die "cannot fork: $!";
            } elsif ($pid == 0) { # child
                my $cmd = "/usr/bin/diff $_ $list{$_}";
                my $date = `/bin/date`;
                die 'date' if ($?);
                chomp $date;
                print STDERR "$date\t$cmd\n";
                system($cmd) && die $cmd;
                exit 0;
            } else { # parent
                die $pid if ($child{$pid});
                $child{$pid} = "$_ $list{$_}";
                last;
            }
        } else { sleep 1 }
    }
}
sleep 1;
foreach (keys %child) {
    print STDERR "waiting for $child{$_}...";
    waitpid($_, 0);
    my $status = $?;
    if ($status) {
        $error = 1;
        print STDERR " FAILED: $! ";
    }
    my $date = `/bin/date`;
    die 'date' if ($?);
    print STDERR " done @ " unless ($status);
    print STDERR $date;
}

exit $error;

################################################################################

sub isIdle {
    return 0 if ($error);
    return 1 unless (keys(%child));
    unless ($CHILDREN_MAX) {
        my $p = `/bin/cat /proc/cpuinfo | /bin/grep processor | /usr/bin/wc -l`;
        die 'cat /proc/cpuinfo' if ($?);
        chomp $p;
        $CHILDREN_MAX = $p;
        $CHILDREN_MAX = 1 unless ($CHILDREN_MAX);
    }
    return 0 if (keys(%child) >= $CHILDREN_MAX);
    my @mpstat = `/usr/bin/mpstat -P ALL 1 1`;
    die 'mpstat' if ($?);
    foreach (@mpstat) {
        chomp;
        @_ = split;
        next unless ($#_ == 10);
        next if ($_[0] eq 'Average:' || $_[1] eq 'CPU' || $_[1] eq 'all');
        my $idle = $_[9];
        return $_ if ($idle > 95);
    }
    return 0;
}

sub Pair {
    my ($p1, $p2) = @_;

    my @tmp = glob("$p1*");
    die "$p1 not found" unless (@tmp);

    my %l1;
    foreach (@tmp) {
        die $_ unless (/^$p1(.*)$/);
        die $l1{$1} if ($l1{$1});
        $l1{$1} = $_;
    }

    @tmp = glob("$p2*");
    die "$p2 not found" unless (@tmp);
    my %l2;
    foreach (@tmp) {
        die $_ unless (/^$p2(.*)$/);
        die $l2{$1} if ($l2{$1});
        $l2{$1} = $_;
    }

    die "different number of $options{1}* vs. $options{2}*"
        if (keys %l1 != keys %l2);

    my %list;
    foreach (keys %l1) {
        die "$options{2}$_ not found" unless ($l2{$_});
        die $l1{$_} if ($list{$l1{$_}});
        $list{$l1{$_}} = $l2{$_};
    }
    
    return %list;
}

################################################################################
# EOF #
