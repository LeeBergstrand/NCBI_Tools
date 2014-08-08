#!/opt/perl-5.8.8/bin/perl -w

use strict;

my $VDB_COPY_TEST;# = 1;

use FindBin qw($Bin);
require "$Bin/common.pl";

use lib '/export/home/sybase/clients-mssql/current64/perl';

use DBI;
use Getopt::Long;
use XML::DOM;

use Cwd 'abs_path';

# acc
# bin
# clean_src
# copycat
# dir
# extra_args
# guess_loader
# help
# loader
# output (just in load-from-db; --out in all other scripts)
# published
# src
# xml

sub Usage { print STDERR "Usage: $0 --acc <accession> --bin <sra_bin_dir> --dir <work_dir> [--guess_loader] --output <output_path>\n"; exit(1); }
my %options;
Usage() unless (GetOptions(\%options, "acc=s", "bin=s", "dir=s", "guess_loader", "help", "output=s"));
Usage() if ($options{help});
Usage() unless ($options{acc} && $options{bin} && $options{dir} && $options{output});

if (-e $options{output})
{   $options{output} = abs_path($options{output}); }

if (-e $options{bin}) {
    $options{bin} = abs_path($options{bin});
} else { die "cannot find $options{bin}"; }

STDOUT->autoflush(1); # autoflush

SetEnv();

my $TAG = "LoadFromDb";

print "<$TAG app=\"" . GetAppName() . "\" " . Timestamp() . " pid=\"$$\">\n";
my $res = 1;
eval {
    $res = Main();
};
if ($@) {
    LogError($@);
    $res = 1 if ($res == 0);
}
print "</$TAG>\n";
exit $res;

################################################################################
sub err_handler {
    my($err, $sev, $state, $line, $server, $proc, $msg, $sql, $err_type) = @_;
    Error($msg, $proc)
    	unless ($err == 99 && $sev == 9 && $state == 0 && $line == 0 &&
		! $server && ! $proc && $msg eq
       "Some character(s) could not be converted into client's character set.  "
		   . "Unconverted bytes were changed to question marks ('?')" &&
		$sql =~ /^exec SRA_Track..LOADER_get_meta .+$/ &&
		$err_type eq 'client');
    1
}

sub Main {
    unless (-e $options{dir})
    {   mkdir $options{dir} || Error("cannot mkdir $options{dir}: $!"); }
    unless (HasError())
    {   chdir $options{dir} || Error("cannot cd $options{dir}: $!"); }

    my $db;

    unless (HasError()) {
        my $server = "NIHMS2";
        $db = DBI->connect("dbi:Sybase:server=$server", "anyone", "allowed", { syb_err_handler => \&err_handler });
        Error("DB problem occured", "Cannot connect to $server") unless ($db);
    }

    unless (HasError()) {
        my $SIZE = 0x140000; # 0xC0000; # 0xA0000 # 0x37000 = 112640 # 0x1D000 = 59392 # 0x1B000 = 55296 # 0x154E6 = 43635
        my $sql = "set textsize $SIZE";
        LogSql($sql);
        $db->do($sql) || Error("Cannot '$sql' : $db->errstr");
    }

    my $runXml;
    my @res;
    unless (HasError()) {
        my $sql = "exec SRA_Track..LOADER_get_meta $options{acc}";
        LogSql($sql);
       #my @res = $db->selectrow_array($sql);
        my $sth = $db->prepare($sql) || Error("DB problem occured", "'$sql' prepare error: $db->errstr");
        unless (HasError()) {
            my $rv  = $sth->execute      || Error("DB problem occured", "'$sql' execute error: $sth->errstr");
            Error("DB problem occured", "'$sql' execute result error") unless ($rv);
        }
        #check error from callback; remove dies
        my @res;
        unless (HasError()) {
            @res = $sth->fetchrow_array;
            if ($sth->{NUM_OF_FIELDS} == 1) {
                if ($sth->{NAME}->[0] eq 'error') {
                    Error("DB problem occured", "$sql returned $res[0]")
                } else { Error("DB problem occured", "$sql returned unexpected FIELDS") }
            } elsif ($sth->{NUM_OF_FIELDS} == 3) {
                unless ($sth->{NAME}->[0] eq 'run' && $sth->{NAME}->[1] eq 'experiment' && $sth->{NAME}->[2] eq 'loader')
                {   Error("DB problem occured", "$sql returned unexpected FIELDS") }
            } else { Error("DB problem occured", "$sql returned unexpected NUM_OF_FIELDS") }
            Error("DB problem occured", "cannot retrieve '$sql' results") unless ($res[0] && $res[1]);
        }
        unless (HasError())
        {   $runXml = $res[0]; }

if ($VDB_COPY_TEST) {
$runXml='<RUN alias="5606" center_name="JGI" run_center="JGI" accession="SRR074861"><EXPERIMENT_REF accession="SRX032832" refname="5604"/><DATA_BLOCK name="61JA3AAXX" sector="2" region="0"><FILES><FILE cc_path="839.2.1026.srf" upload_id="406846" file_id="355904" checksum="d38a7a4536710830879c740895151126" file_cc_id="1" checksum_method="MD5" filename="SRR000221" filetype="sra"/><!--kar--></FILES></DATA_BLOCK><RUN_ATTRIBUTES><RUN_ATTRIBUTE><TAG>Run ID</TAG><VALUE>839</VALUE></RUN_ATTRIBUTE></RUN_ATTRIBUTES></RUN>';
delete $res[2];
}

        unless (HasError()) {
            my $name = "run.xml";
            open(OUT, ">$name") || Error("cannot open $name to write: $!");
            unless (HasError()) {
                print OUT $runXml;
                close OUT || Error("cannot close >$name: $!");
            }
        }
        unless (HasError()) {
            my $name = "experiment.xml";
            open(OUT, ">$name") || Error("cannot open $name to write: $!");
            unless (HasError()) {
                print OUT $res[1];
                close OUT || Error("cannot close >$name: $!");
            }
        }
        unless (HasError() || $options{guess_loader}) {
            if ($res[2]) {
                $options{loader} = $res[2];
            } else {
                my $noNeedLoader;
                if (NeedFixXml($runXml))
                {   $runXml = FixXml("run.xml", "run.xml.orig", $runXml); }
                my $parser = new XML::DOM::Parser;
                eval {
                    my $dRun = $parser->parsestring($runXml);
                    my @FILES = $dRun->getElementsByTagName('FILE');
                    foreach my $node (@FILES) {
                        my $f = $node->getAttribute('filetype');
                        unless ($f eq 'sra' && $f eq 'kar') {
                            ++$noNeedLoader;
                            Error("Multiple FILE nodes for filetype=$f") unless ($#FILES == 0);
                            last;
                        }
                    }
                };
                if ($@)
                {   Error("run.xml: $@"); }
                Error("DB problem occured", "'$sql' did not return the loader") unless ($noNeedLoader);
            }
        }
    }

    unless (HasError()) {
        my $sql = "exec SRA_Track..LOADER_get_uploads $options{acc}";
        LogSql($sql);
        @res = $db->selectrow_array($sql);
        unless (HasError())
        {   Error("DB problem occured", "cannot retrieve '$sql' results") unless ($res[0]); }
    }

    unless (HasError()) {
        my $name = "uploads.xml";
        open(OUT, ">$name") || Error("cannot open $name to write: $!");
        unless (HasError()) {
$res[0] = '<uploads-list acc="SRR074861"><upload upload_id="406846" file_path="/panfs/traces01/sra0/SRR/000000/SRR000221" CC_path="/.autofsck" unpack="0"/></uploads-list>' if ($VDB_COPY_TEST);
            print OUT $res[0];
            close OUT || Error("cannot close >$name: $!");
        }
    }

    # detect whether the run is published (released)
    if (0) {
        my $sql = "select case when (published <= getdate() and files_processed=1) then 1 else 0 end " .
            "from SRA_Main..Run where acc = '$options{acc}'";
        LogSql($sql);
        @res = $db->selectrow_array($sql);
        unless (HasError())
        {   Error("DB problem occured", "cannot retrieve '$sql' results") unless (@res && $#res == 0 && defined $res[0]) }
        unless (HasError())
        {   $options{published} = $res[0]; }
    }

    my $res = 1;
    unless (HasError()) {
        my $cmd = "$Bin/load-from-dir.pl -b $options{bin} -c -o $options{output}";
        if ($options{loader})
        {   $cmd .= " -l $options{loader}"; }
        if (defined $options{published})
        {   $cmd .= " -p $options{published}"; }
        LogExec($cmd, 'noClose');
        my $rc = system($cmd);
        $res = $rc >> 8;
        if ($res)
        {   Error("failed to run load-from-dir.pl"); }
        print "</Exec>\n";
    }
    return $res;
}
