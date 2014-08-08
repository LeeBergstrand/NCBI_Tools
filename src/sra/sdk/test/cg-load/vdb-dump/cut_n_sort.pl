#!/usr/local/bin/perl -w
################################################################################

use strict;

use File::Basename 'basename';
use FindBin '$Bin';
use Getopt::Long 'GetOptions';
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
  $appname --fields <list> --input <in-files-prefix> --output <out-files-prefix>
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options,
    "fields=s", "help", "input=s", "output=s"));
Help(0) if ($options{help});
Help(1) unless ($options{fields} && $options{input} && $options{output});

my $prefix = $options{input};
my @in = glob("$prefix*");
die "$prefix not found" unless (@in);

#STDOUT->autoflush(1);
#STDERR->autoflush(1);

print STDERR "@in";
print STDERR "\n";

my $CHILDREN_MAX;
my %child;
my $error = 0;
foreach (@in) {
    die $_ unless (/^$prefix(.*)$/);
    while (1) {
        die "cannot find $_" unless (-e $_);
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
                my $cmd = "/bin/cut        -f$options{fields} $_"
                 . " PIPE /bin/sort > $options{output}$1";
                $cmd = "$Bin/cut_n_sort.sh -f$options{fields} $_"
                                  . " $options{output}$1";
                system($cmd) && die $cmd;
                exit 0;
            } else { # parent
                die $pid if ($child{$pid});
                $child{$pid} = $_;
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

sub isIdle {
    return 0 if ($error);
    return 1 unless (keys(%child));
    unless ($CHILDREN_MAX) {
        my $p = `/bin/cat /proc/cpuinfo | /bin/grep processor | /usr/bin/wc -l`;
        die 'cat /proc/cpuinfo' if ($?);
        chomp $p;
        $CHILDREN_MAX = $p / 2;
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

################################################################################
# EOF #
