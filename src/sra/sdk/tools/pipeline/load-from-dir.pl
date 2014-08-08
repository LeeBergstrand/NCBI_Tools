#!/opt/perl-5.8.8/bin/perl -w

# TODO : the load fails if output_path's parent directory does not exist

use strict;

use FindBin qw($Bin);
require "$Bin/common.pl";

use Getopt::Long;
use XML::DOM;

use Cwd;
use Cwd 'abs_path';

my $TAG = "LoadFromDir";

local $/=undef;

sub Usage {
  print STDERR "Usage: $0 --bin <sra_bin_dir> [--clean_src] [--dir <work_dir>] [--extra_args <loaded-extra-command-line-args>] " .
                   "[--help] [--loader <loader>] --out <output_path> [--published 0|1]\n";
  exit(1);
}

my %options;
Usage() unless (GetOptions(\%options, "extra_args=s", "bin=s", "clean_src", "dir=s", "help", "loader=s", "out=s", "published=i"));
Usage() if ($options{help});
Usage() unless ($options{bin} && $options{out});

my $parser = new XML::DOM::Parser;

# update all paths to absolute
{
    $options{bin} = abs_path($options{bin});
    $options{dir} = abs_path($options{dir}) if ($options{dir});
    if (-e $options{out})
    {   $options{out} = abs_path($options{out}); }
}

SetEnv();

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

sub Main {
    my $res = 1;
    if ($options{dir})
    {   Chdir($options{dir}); }

    my $upload_id;
    unless (HasError()) {
        # make sure (experiment&run).xml will not break the loader
        my $experiment = ReadAndFixXml("experiment.xml");
        my $run        = ReadAndFixXml("run.xml");

        # [guess loader]
        unless (HasError() || $options{loader})
        {   ($options{loader}, $upload_id) = GuessLoader($experiment, $run); }
    }

    # parse uploads xml
    my $UPLOADS = "uploads.xml";
    my @uploads;
    my $upcompressed;
    my $src;
    if (!HasError() && -e $UPLOADS) {
        open (U, $UPLOADS) || Error("Cannot open $UPLOADS");
        unless (HasError()) {
            my $in = <U>;
            eval {
                my $uploadsDoc = $parser->parsestring($in);
                @uploads = $uploadsDoc->getElementsByTagName('upload');
            };
            if ($@)
            {   Error("$UPLOADS: $@"); }
            unless (HasError())
            {   Error("cannot find 'upload' in uploads.xml") unless (@uploads && $#uploads >= 0); }
        }

        # prepare sources, unpack if necessary
        $src = "src";
        unless (HasError()) {
            if (-e $src) {
                my $cmd = "rm -rf $src";
                if ($options{clean_src}) {
                    my $rc = system($cmd);
                    if ($rc) {
                        Error("failed to run '$cmd': $!");
                        $res = $rc >> 8;
                    }
                } else { Error("$src exists in " . getcwd() . ": use --clean_src option to remove it"); }
            }
        }
        unless (HasError() || -e $src)
        {   mkdir $src || Error("cannot mkdir $src: $!"); }
        unless (HasError())
        {   Chdir($src); }
    }

    my $file_path;
    unless (HasError()) {
        my $uncompressedCreated;
        foreach my $upload (@uploads) {
            my $id     = $upload->getAttribute('upload_id');
            CheckAttr($id, "upload_id", $id);
            my $cc_xml = $upload->getAttribute('CC_path');
            CheckAttr($id, "CC_path", $cc_xml);

            my $file   = $upload->getAttribute('file_path');
            CheckAttr($id, "file_path", $file);
            if ($file && $id && $upload_id && $id eq $upload_id)
            {   $file_path = $file; }

            my $unpack = $upload->getAttribute('unpack');

            unless (HasError()) {
                # not needed when uncompressed, just FYI
                my $upload_dir = dirname($file);
                CheckExist($file);
                CheckExist($cc_xml);
                unless (HasError())
                {   symlink $upload_dir, $id   || Error("cannot ln -s $upload_dir, $id"); }
                unless (HasError())
                {   symlink $cc_xml, "$id.xml" || Error("cannot ln -s $cc_xml, $id.xml"); }
            }

            if (!HasError() && $unpack) {
                $upcompressed = "uncompressed";
                unless ($uncompressedCreated) {
                    ++$uncompressedCreated;
                    $src .= "/$upcompressed";
                    unless (-e $upcompressed)
                    {   mkdir $upcompressed || Error("cannot mkdir $upcompressed: $!"); }
                }
                unless (HasError()) {
                    $cc_xml = "$upcompressed/$id.xml";
                    my $cmd = "$Bin/uncompress.pl -c $options{bin}/copycat -o $upcompressed/$id -s '$file' -x $cc_xml";
                    LogExec($cmd);
                    my $rc = system($cmd);
                    if ($rc) {
                        Error("failed to run uncompress.pl");
                        $res = $rc >> 8;
                    }
                }
            }
        }
    }

    if (!HasError() && @uploads)
    {   Chdir(".."); }

    # load
    unless (HasError()) {
        unless ($options{loader} eq 'vdb-copy') {
            my $cmd = "$Bin/run-loader.pl -l $options{bin}/$options{loader} -o $options{out} -b $options{bin}";
            if ($src)
            {  $cmd .= " -s $src"; }
            if ($options{extra_args})
            {   $cmd .= " -e '$options{extra_args}'"; }
            if ($options{published})
            {   $cmd .= " -p $options{published}"; }
            LogExec($cmd, 'noClose');
            my $rc = system($cmd);
            $res = $rc >> 8;
            if ($res)
            {   Error("failed to run run-loader.pl"); }
            print "</Exec>\n";
        }
        # vdb-copy land
        elsif ($file_path) {
            my $cmd = "$Bin/run-install.pl -b $options{bin} -s '$file_path' -o $options{out}"; # -e '-f -u'";
            $cmd .= " -e $options{extra_args}" if ($options{extra_args});
            LogExec($cmd, 'noClose');
            my $rc = system($cmd);
            $res = $rc >> 8;
            if ($res)
            {   Error("failed to run run-loader.pl"); }
            print "</Exec>\n";
        } else { Error("could not find source path to run vdb-copy"); }
    }
    unless (HasError()) {
        my $cmd = "$options{bin}/vdb-lock $options{out}";
        LogExec($cmd);
        my $rc = system($cmd);
        Error("cannot lock $options{out}") if ($rc);
        $res = $rc >> 8;
    }

    return $res;
}

sub CheckExist {
    my ($file) = @_;
    LogCheck($file);
    Error("cannot find $file") unless (-e $file);
}

sub CheckAttr {
    my ($id, $name, $val) = @_;
    Error("Cannot find $name for id = '$id'") unless ($val);
}

sub ReadAndFixXml {
    my ($name) = @_;
    my $xml;
    open(IN, $name) || Error("Cannot open $name: $!");
    unless (HasError()) {
        $xml = <IN>;
        close(IN) || Error("cannot close $name: $!");
        if (!HasError() && NeedFixXml($xml))
        {   $xml = FixXml($name, "$name.bak", $xml); }
    }
    $xml;
}

sub GuessLoader {
    my ($experiment, $run) = @_;
    
    my ($p, $t, $loader, $upload_id);
    eval {
        my $dExperiment = $parser->parsestring($experiment);
        foreach my $node ($dExperiment->getElementsByTagName('PLATFORM')) {
            my @p = $node->getChildNodes();
            foreach (@p) {
                if ($_->getNodeType() == ELEMENT_NODE) {
                    $p = $_->getNodeName();
                    last;
                }
            }
        }
    };
    if ($@)
    {   Error("experiment.xml: $@"); }
    elsif (!$p)
    {   Error("experiment.xml: cannot get PLATFORM"); }
    else {
        unless (HasError()) {
            eval {
                my $dRun = $parser->parsestring($run);
                my @FILES = $dRun->getElementsByTagName('FILE');
                foreach my $node (@FILES) {
                    my $f = $node->getAttribute('filetype');
                    if ($f eq 'sra' || $f eq 'kar') {
                        if ($#FILES == 0) {
                            $upload_id = $node->getAttribute('upload_id');
                        } else { Error("Multiple FILE nodes for filetype=$f"); }
                    } elsif ($t && $f ne $t) {
                        if ($t eq '454_native_seq' && $f eq '454_native_qual') {
                        } elsif ($t eq 'SOLiD_native_csfasta' && $f eq 'SOLiD_native_qual') {
                        } elsif ($t eq 'SOLiD_native_qual' && $f eq 'SOLiD_native_csfasta') {
                        } else {
                            Error("incompatible file types in run.xml: $t vs. $f");
                            last;
                        }
                    }
                    $t = $f;
                }
            };
            if (!HasError() && $@)
            {   Error("run.xml: $@"); }
        }
    }

    unless (HasError() || $t)
    {   Error("run.xml: cannot get filetype"); }

    unless (HasError()) {
        if ($t eq 'kar' ||
            $t eq 'sra')
        {
            $loader = 'vdb-copy';
        } elsif ($p eq 'ABI_SOLID') {
            if ($t eq 'fastq') {
                $loader = "fastq-load";
            } elsif ($t eq 'SOLiD_native') {
                $loader = "abi-load";
            } elsif ($t eq 'SOLiD_native_csfasta') {
                $loader = "abi-load";
            } elsif ($t eq 'SOLiD_native_qual') {
                $loader = "abi-load";
            } elsif ($t eq 'srf') {
                $loader = "srf-load";
            } else { Error("unexpected: $t/$p"); }
        } elsif ($p eq 'HELICOS') {
            if ($t eq 'fastq') {
                $loader = "helicos-load";
            } elsif ($t eq 'Helicos_native') {
                $loader = "helicos-load";
            } else { Error("unexpected: $t/$p"); }
        } elsif ($p eq 'ILLUMINA') {
            if ($t eq '_seq.txt, _prb.txt, _sig2.txt, _qhg.txt') {
                $loader = "illumina-load";
            } elsif ($t eq 'Illumina_native') {
                $loader = "illumina-load";
            } elsif ($t eq 'Illumina_native_qseq') {
                $loader = "illumina-load";
            } elsif ($t eq 'fastq') {
                $loader = "fastq-load";
            } elsif ($t eq 'srf') {
                $loader = "srf-load";
            } else { Error("unexpected: $t/$p"); }
        } elsif ($p eq 'LS454') {
            if ($t eq 'fastq') {
                $loader = "fastq-load";
            } elsif ($t eq 'sff') {
                $loader = "sff-load";
            } else { Error("unexpected: $t/$p"); }
        } else { Error("unexpected PLATFORM='$p'") }
    }

    ($loader, $upload_id);
}
