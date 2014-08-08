#!/opt/perl-5.8.8/bin/perl -w

use strict;

use FindBin qw($Bin);
require "$Bin/common.pl";

use Fcntl ':mode';
use Getopt::Long;

use POSIX qw <F_SETFD F_GETFD FD_CLOEXEC>;

my $LOADER_OPT = "";

sub Usage {
  print STDERR "Usage: $0 [--bin <sra_bin_dir>] [--dir <work_dir>] --loader <loader> --out <output_path> [--published 0|1] [--src <source-dir>]\n";
  exit(1);
}

my %options;
Usage() unless (GetOptions(\%options, "bin=s", "dir=s", "help", "extra_args=s", "loader=s", "out=s", "published=i", "src=s"));
Usage() if ($options{help});
Usage() unless ($options{loader} && $options{out});

die "Cannor find $options{loader}" unless (-e $options{loader});

if ($options{extra_args})
{   $LOADER_OPT .= " $options{extra_args} "; }

select((select(STDOUT), $| = 1)[0]); # autoflush

SetEnv();

print "<Load app=\"" . GetAppName() . "\" " . Timestamp() . " pid=\"$$\">\n";
my $res = 1;
eval {
    $res = Main();
};
if ($@) {
    LogError($@);
    $res = 1 if ($res == 0);
}
print "</Load>\n";
exit $res;

sub Main {
    my $rc = 0;
    if ($options{dir})
    {   Chdir($options{dir}) }
    if (HasError())
    {   $rc = 1 << 8; }
    if ($rc == 0 && -e $options{out}) {
        my $cmd = "chmod -R +w $options{out} && rm -R $options{out}";
        LogExec($cmd);
        $rc = system($cmd);
        Error("cannot remove $options{out}: $!") if ($rc);
    }

    unless ($rc) {
    	my $pacbio_load = "pacbio-load";
    	unless (-e $pacbio_load) {
          	symlink "$options{bin}/pacbio-load", $pacbio_load
	  		or Error("cannot ln -s $pacbio_load");
	}
        $rc = LoadPipe();
    }
    my $res = $rc >> 8;
}

sub LoadPipe {
    if ($options{src}) {
        unless (-e $options{src}) {
           Error("cannot find '$options{src}'");
           return 1 << 8;
        }
    }
    unless (pipe(FROM_CHILD, TO_PARENT)) {
        Error("cannot create pipe: $!");
        return 1 << 8;
    }
    select((select(TO_PARENT), $| = 1)[0]); # autoflush
    if (!defined(my $pid = fork)) {
        Error("cannot fork: $!");
    } elsif ($pid) { # parent
        close TO_PARENT;

        my $name = "log.xml";
        open(LOG, "> $name") || Error("cannot open $name");
        unless (HasError()) {
            select((select(LOG), $|=1)[0]); # autoflush log.xml
            print LOG "<Log>\n";
            while (<FROM_CHILD>) {
	        if ( $_ !~ /Log>/ ) {
                   print ;
                   print LOG $_;
                }
            }
            print LOG "</Log>\n";
            close(LOG) || Error("cannot close $name");
        }

        waitpid($pid, 0);
        print "</Exec>\n";
        if ($?) {
            return $?;
        } elsif (HasError()) {
            return 1 << 8;
        } else { return 0; }
    } else { # child
        close FROM_CHILD;
        my $flags = fcntl TO_PARENT, F_GETFD, 0;
        $flags &= ~FD_CLOEXEC;
        fcntl TO_PARENT, F_SETFD, $flags;
        $_ = "$options{loader} --xml-log-fd " . fileno(TO_PARENT)
          . " $LOADER_OPT -e experiment.xml -L info -o $options{out} -r run.xml"
          . " --input-no-threads" ;
        if ($options{src})
        {   $_ .= " -i $options{src}"; }
        LogExec($_, 'noClose');
        my $RUNNER = "cmd.sh";
        open CMD, ">$RUNNER" || Suicide("cannot open $RUNNER: $!");
        print CMD "umask 7\n";
        print CMD "export PATH=$options{bin}:\$PATH\n";
        print CMD "$_\n";
        close CMD || Suicide("cannot close $RUNNER: $!");
        my @args = ("sh", $RUNNER);
        close STDOUT || Suicide("cannot close STDOUT: $!");
        exec(@args);
        exit(0);
    }
}

################################################################################

sub Suicide {
    Error($_[0]);
    exit(1);
}
