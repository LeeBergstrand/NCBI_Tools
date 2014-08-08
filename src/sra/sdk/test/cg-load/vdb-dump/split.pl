#!/usr/local/bin/perl -w
################################################################################

use strict;

use File::Basename 'basename';
use Getopt::Long "GetOptions";

my $appname = basename($0);
sub Help {
    my ($exit) = @_;
    $exit = 0 unless (defined $exit);
    my $stream = *STDOUT;
    $stream = *STDERR if ($exit);
    print $stream <<EndText;
Usage:
    $appname --field <number> --output <out-path-prefix>
EndText

    exit $exit;
}

my %options;
Help(1) unless (GetOptions(\%options, "field=n", "help", "output=s"));
Help(0) if ($options{help});
Help(1) unless ($options{field} && $options{output});
--$options{field};

my %file;

my $HEADER = '';
while (<>)
{   Print($_) }

close $file{$_} foreach (keys %file);

sub Print {
    ($_) = @_;
    if (/^>/) {
        $HEADER = $_;
        return;
    }
    @_ = split();
    my $key;
    if ($#_ < $options{field}) {
             $key = 'unaligned';
    } else { $key = $_[$options{field}]; }
    unless ($file{$key}) {
        open $file{$key}, ">$options{output}$key"
            or die "cannot open $options{output}$key";
        print STDERR "$appname: created $options{output}$key\n";
        print { $file{$key} } $HEADER if ($HEADER);
    }
    print { $file{$key} } $_;
}

################################################################################
# EOF #
