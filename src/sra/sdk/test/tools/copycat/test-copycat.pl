#!/usr/bin/perl -w
# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================
use File::Path;
use File::Basename;


my $testbase;
my $verbose = 1;
my $log;
my $error_count;

BEGIN
{
    $testbase = "test-copycat-tmp.$$";
    $error_count = 0;

    unlink ($testbase) or die $! if (-f $testbase);
    rmdir  ($testbase) or die $! if (-d $testbase);

#    print ("+++++\nCreating $testbase to run tests\n+++++\n");
    mkpath ($testbase) or die $!;
    chdir ($testbase) or die $!;
}

END
{
#    print ("+++++\nRemoving $testbase\n+++++\n") if ($errors == 0);


    log_it ("%s\n%s%u\n%s\n",
            "======================================================================",
            "Total error count: ",
            $error_count,
            "======================================================================");
    if ($error_count == 0)
    {
        chdir ("..") or die $!;
        rmtree ($testbase) or die $!;
    }
}

sub log_it
{
    printf{STDERR} (@_) if ($verbose);
    printf{$log} (@_) if defined($log);
}

sub delete_files
{
    while (my $path = shift)
    {
        unlink ("$testbase/$path");#  or die "$testbase/$path not deleted";
    }
}

sub create_dir
{
    my ($path) = @_;

    unlink ($path) or dir $! if (-f $path);

    mkpath ($path);
}

sub create_file
{
    my ($path, $lines, $repeated_value, $linesize) = @_;

    # evaluate parameters die or default as needed
    die "create_file needs a file name" unless defined $path;
    $lines = 0 unless defined $lines;
    $linesize = 0 unless defined $linesize;
    $repeated_value = 0 unless defined $repeated_value;
    my $line;
    my $llinesize;

    # if we have a number of lines and a specified repeat character
    # make our constant line
    if (($lines > 0) && (ord($repeated_value) > 0) && ($linesize > 0))
    {
        $llinesize = ($linesize != 0) ? $linesize : int (rand (80));
        $line = "\n";
        for (my $ix = 0; $ix < $llinesize; ++$ix)
        {
            $line = $repeated_value . $line;
        }
    }

    my $dirname = dirname ($path);

    # we assume the dir name is relative to $testbase
    # we'll unceremoniously die if not

    unless (-d $dirname)
    {
        unlink ($dirname) if (-f $dirname);
        mkpath ($dirname) or die %!;
    }
    unlink ($path) or die $! if (-f $path);

    open (my $FH, '>', $path) or die $!;

    for ( ; $lines > 0; --$lines)
    {
        $llinesize = ($linesize != 0) ? $linesize : int (rand (80));
        $line = "\n";

        for (my $ix = 0; $ix < $llinesize; ++$ix)
        {
            $line = ((ord($repeated_value) > 0)
                     ? $repeated_value
                     : chr (32 + int ( rand (126 - 32)))) . $line;
        }
        print{$FH} ($line);
    }

    close ($FH);
}

sub test_header
{
    my $test = join ("\n",@_);
    log_it ("%s\n%s\n%s\n",
            "======================================================================",
            $test,
            "==========");
}

sub call_system
{
    my @aargs = @_;
    my $cmd = join (' ',@aargs);
    log_it ("-----\n%s\n-----\n",$cmd);
    return system (@aargs);
}

my @capture;
sub system_capture_stdout
{
    my @aargs = @_;
    my $cmd = join (' ',@aargs);
    @capture = ();
    my $linein;
    open (FH, "-|", $cmd) or die $!;

    @capture = ();
    while (($linein = <FH>), defined ($linein))
    {
        push (@capture, $linein);
    }
}
sub call_system_capture_stdout
{
    my @aargs = @_;
    my $cmd = join (' ',@aargs);
    @capture = ();
    my $linein;
    log_it ("-----\n%s\n-----\n",$cmd);
    open (FH, "-|", $cmd) or die $!;

    @capture = ();
    while (($linein = <FH>), defined ($linein))
    {
        push (@capture, $linein);
    }
}

sub error_result
{
    my $error = join ("\n",@_);
    log_it ("%s\n%s\n%s\n",
            "####################",
            $error,
            "####################");
    ++$error_count;
}

sub cc_file_2_file
{
    test_header ("Running a test of simple file to file copy",
                 "and a test of the force option",
                 "",
                 "The first copycat should complete correctly",
                 "The second should fail with path exists",
                 "The third should complete correctly",
                 "The fourth should complete correctly");

    create_file ("source/file",10);

    error_result ("copycat simple copy failed")
        if (call_system ("copycat",
                         "source/file",
                         "destination/file"));

    error_result ("should have failed without force: $?")
        if (! call_system ("copycat",
                           "source/file",
                           "destination/file"));

    error_result ("should have succeeded with force")
        if (call_system ("copycat",
                         "--force",
                         "source/file",
                         "destination/file") ||
            call_system ("copycat",
                         "-f",
                         "source/file",
                         "destination/file"));


    rmtree ("source") or die $! if ($error_count == 0);
    rmtree ("destination") or die $! if ($error_count == 0);
}

sub cc_files_2_dir
{
    test_header ("Running single and multiple files to directory copy",
                 "There is a copy of an empty file but no specific check of result size",
                 "There should be no problems here");
    create_file ("source/file0",0);
    create_file ("source/file1",1);
    create_file ("source/file2",2);
    create_file ("source/file3",3);
    create_file ("source/file4",4);
    create_file ("source/file5",5);
    create_file ("source/file6",6);
    create_file ("source/file7",7);
    create_file ("source/file8",8);
    create_dir ("destination");

    error_result ("copycat files to directory failed")
        if (call_system ("copycat",
                         "source/file0",
                         "destination") ||
            call_system ("copycat",
                         "source/file1",
                         "source/file2",
                         "source/file3",
                         "destination") ||
            call_system ("copycat",
                         "source/file4",
                         "-o",
                         "destination") ||
            call_system ("copycat",
                         "source/file5",
                         "source/file6",
                         "source/file7",
                         "source/file8",
                         "--output=destination") ||
            ( ! -f "destination/file0") ||
            ( ! -f "destination/file1") ||
            ( ! -f "destination/file2") ||
            ( ! -f "destination/file3") ||
            ( ! -f "destination/file4") ||
            ( ! -f "destination/file5") ||
            ( ! -f "destination/file6") ||
            ( ! -f "destination/file7") ||
            ( ! -f "destination/file8"));

    rmtree ("source") or die $! if ($error_count == 0);
    rmtree ("destination") or die $! if ($error_count == 0);
}

sub cc_files_2_devnull
{
    test_header ("Running single and multiple files to nowhere copy",
                 "There should be no problems here");
    create_file ("source/file1",100);
    create_file ("source/file2",110);
    create_file ("source/file3",120);

    error_result ("copycat to nowhere copy failed")
        if (call_system ("copycat",
                         "source/file1",
                         "source/file2",
                         "source/file3",
                         "/dev/null") ||
            call_system ("copycat",
                         "source/file1",
                         "source/file2",
                         "source/file3",
                         "/dev/null"));

    rmtree ("source") or die $! if ($error_count == 0);
}

sub cc_files_2_stdout
{
    test_header ("Running copy of file to stdout",
                 "There should be no problems here");
    create_file ("source/file",12);

    error_result ("copycat to std copy failed")
        if (call_system_capture_stdout ("copycat",
                                        "source/file",
                                        "/dev/stdout"));
    @capture = ();

    rmtree ("source") or die $! if ($error_count == 0);
}


sub cc_gzip_file
{
    test_header ("Testing handling of gzip file",
                 "We create two gzip files that have",
                 "different exteriors but should have",
                 "the same interiors",
                 "We are testing the concatenation of",
                 "zipped files here",
                 "The first call to copycat will succeed",
                 "while the second should encounter 'data insufficient' errors");

    create_file ("source/fileA",10,'A');
    create_file ("source/fileB",10,'B');
    create_file ("source/fileC",10,'C');
    create_file ("source/fileD",10,'D');
    create_file ("source/fileE",10,'E');

    system_capture_stdout
        ("cat",
         "source/fileA",
         "source/fileB",
         "source/fileC",
         "source/fileD",
         "source/fileE",
         "source/fileB",
         "source/fileC",
         "source/fileD",
         "source/fileE",
         "source/fileA");

    open (FH,">", "source/file");
    foreach $line (@capture)
    {
        print{FH} $line;
    }
    close (FH);
    system ("gzip","source/file");
    system ("gzip","source/fileA");
    system ("gzip","source/fileB");
    system ("gzip","source/fileC");
    system ("gzip","source/fileD");
    system ("gzip","source/fileE");

    system_capture_stdout
        ("cat",
         "source/fileA.gz",
         "source/fileB.gz",
         "source/fileC.gz",
         "source/fileD.gz",
         "source/fileE.gz",
         "source/fileB.gz",
         "source/fileC.gz",
         "source/fileD.gz",
         "source/fileE.gz",
         "source/fileA.gz");
    open (FH,">", "source/file.cat.gz");
    foreach $line (@capture)
    {
        print{FH} $line;
    }
    close (FH);

    error_result ("copycat on gzip files failed")
        if (call_system ("copycat",
                         "source/file.gz",
                         "source/file.cat.gz",
                         "/dev/null"));

    my $z = -s "source/file.gz";

    open (FH, "+<","source/file.gz");
    truncate (FH, int ($z/2));
    close (FH);

    $z = -s "source/file.cat.gz";

    open (FH, "+<","source/file.cat.gz");
    truncate (FH, int ($z/2));
    close (FH);

    error_result ("copycat on gzip files failed")
        if (call_system ("copycat",
                         "source/file.gz",
                         "source/file.cat.gz",
                         "/dev/null"));

    rmtree ("source") or die $! if ($error_count == 0);
}

sub cc_bzip2_file
{
    test_header ("Testing handling of bzip2 file",
                 "We create two bzip2 files that have",
                 "different exteriors but should have",
                 "the same interiors",
                 "We are testing the concatenation of",
                 "zipped files here",
                 "The first call to copycat will succeed",
                 "while the second should encounter 'data insufficient' errors");

    create_file ("source/fileA",10,'A');
    create_file ("source/fileB",10,'B');
    create_file ("source/fileC",10,'C');
    create_file ("source/fileD",10,'D');
    create_file ("source/fileE",10,'E');

    system_capture_stdout
        ("cat",
         "source/fileA",
         "source/fileB",
         "source/fileC",
         "source/fileD",
         "source/fileE",
         "source/fileB",
         "source/fileC",
         "source/fileD",
         "source/fileE",
         "source/fileA");

    open (FH,">", "source/file");
    foreach $line (@capture)
    {
        print{FH} $line;
    }
    close (FH);
    system ("bzip2","source/file");
    system ("bzip2","source/fileA");
    system ("bzip2","source/fileB");
    system ("bzip2","source/fileC");
    system ("bzip2","source/fileD");
    system ("bzip2","source/fileE");

    system_capture_stdout
        ("cat",
         "source/fileA.bz2",
         "source/fileB.bz2",
         "source/fileC.bz2",
         "source/fileD.bz2",
         "source/fileE.bz2",
         "source/fileB.bz2",
         "source/fileC.bz2",
         "source/fileD.bz2",
         "source/fileE.bz2",
         "source/fileA.bz2");
    open (FH,">", "source/file.cat.bz2");
    foreach $line (@capture)
    {
        print{FH} $line;
    }
    close (FH);

    error_result ("copycat on bzip2 files failed")
        if (call_system ("copycat",
                         "source/file.bz2",
                         "source/file.cat.bz2",
                         "/dev/null"));

    my $z = -s "source/file.bz2";

    open (FH, "+<","source/file.bz2");
    truncate (FH, int ($z/2));
    close (FH);

    $z = -s "source/file.cat.bz2";

    open (FH, "+<","source/file.cat.bz2");
    truncate (FH, int ($z/2));
    close (FH);

    error_result ("copycat on bzip2 files failed")
        if (call_system ("copycat",
                         "source/file.bz2",
                         "source/file.cat.bz2",
                         "/dev/null"));

    rmtree ("source") or die $! if ($error_count == 0);
}

sub cc_archive_file
{
    test_header ("Testing handling of tar files",
                 "We create two gzip files that have",
                 "different exteriors but should have",
                 "the same interiors",
                 "We are testing the concatenation of",
                 "zipped files here",
                 "The first call to copycat will succeed",
                 "while the second should encounter 'data insufficient' errors");

    create_file ("source/dir/fileA",10,'A');
    create_file ("source/dir/fileB",10,'B');
    create_file ("source/dir/fileC",10,'C');
    create_file ("source/dir/fileD",10,'D');
    create_file ("source/dir/fileE",10,'E');

    system ("tar",
            "-cvf",
            "source/dir.tar",
            "source/dir");

    system ("kar",
            "-c",
            "source/dir.sra",
            "-d",
            "source/dir");

    system ("kar",
            "-c",
            "source/dir.kar",
            "-d",
            "source/dir");



    error_result ("copycat on tar files failed")
        if (call_system ("copycat",
                         "source/dir.tar",
                         "/dev/null"));

    error_result ("copycat on kar files failed")
        if (call_system ("copycat",
                         "source/dir.sra",
                         "source/dir.kar",
                         "/dev/null"));

    my $z = -s "source/dir.tar";

    open (FH, "+<","source/dir.tar");
    truncate (FH, int ($z/8));
    close (FH);

    $z = -s "source/dir.kar";

    open (FH, "+<","source/dir.kar");
    truncate (FH, int ($z/8));
    close (FH);

    error_result ("copycat on tar files failed")
        if (call_system ("copycat",
                         "source/dir.tar",
                         "/dev/null"));

    error_result ("copycat on kar files failed")
        if (call_system ("copycat",
                         "source/dir.kar",
                         "/dev/null"));

    rmtree ("source") or die $! if ($error_count == 0);
}

sub cc_ncbi_enc
{
    test_header ("Running simple limited test of old style encryption",
                 "copycat can not create this encryption format and",
                 "will not be made to do so as it has too many weaknesses.",
                 "This test uses an external file unlike most(all) of the",
                 "other tests. - this greatly reduces flexibility in running",
                 "this particular test.");

    print ("Don't have any files to use yet.\n");


}

sub cc_nenc_file
{
    test_header ("Running a test of handling of encrypted files",
                 "The last compycat should warn about not decoding",
                 "and the last diff should say files differ");

    create_file ("source/file",1000);
    create_file ("source/password_A",1,'A',32);
    create_file ("source/password_B",1,'B',32);

    system ("nenctool",
            "source/file",
            "ncbi-file:source/file.A.nenc?enc&pwfile=source/password_A");

    system ("nenctool",
            "source/file",
            "ncbi-file:source/file.B.nenc?enc&pwfile=source/password_B");

    error_result ("Failed to decrypt input file")
        if (call_system ("copycat",
                         "ncbi-file:source/file.A.nenc?enc&pwfile=source/password_A",
                         "destination/file") ||
            call_system ("copycat",
                         "source/file",
                         "ncbi-file:destination/file.A.nenc?enc&pwfile=source/password_A") ||
            call_system ("copycat",
                         "ncbi-file:source/file.A.nenc?enc&pwfile=source/password_A",
                         "ncbi-file:destination/file.B.nenc?enc&pwfile=source/password_B") ||
            call_system ("copycat",
                         "source/file.B.nenc",
                         "destination/file.B") ||
            call_system ("diff",
                         "source/file",
                         "destination/file") ||
            call_system ("diff",
                         "source/file.A.nenc",
                         "destination/file.A.nenc") ||
            call_system ("diff",
                         "source/file.B.nenc",
                         "destination/file.B.nenc") ||
            ( ! call_system ("diff",
                             "source/file.B.nenc",
                             "destination/file.A.nenc")));

    rmtree ("source") or die $! if ($error_count == 0);
    rmtree ("destination") or die $! if ($error_count == 0);
}

sub cc_extract
{
    test_header ("Running a test of the extract feature",
                 "It will also test some combining various file types",
                 "The --extract-to-dir / -E will also be tested.");

    create_file ("source/dir/text.A",1000,'A');
    create_file ("source/dir/text.B",1000,'A');

    system ("bzip2",
            "source/dir/text.A");
    system ("bzip2",
            "source/dir/text.B");

    system ("tar",
            "-cvf",
            "source/dir.tar",
            "source/dir");

    system ("bunzip2",
            "source/dir/text.A.bz2");
    system ("bunzip2",
            "source/dir/text.B.bz2");

    system ("gzip","source/dir.tar");

    system ("cp",
            "source/dir.tar.gz",
            "source/dir.tgz");

    error_result ("copycat extract failure")
        if (call_system ("copycat",
                         "-e",
                         "e",
                         "source/dir.tar.gz",
                         "destination/") ||
            call_system ("copycat",
                         "--extract",
                         "extract",
                         "source/dir.tgz",
                         "/dev/null") ||
            call_system ("copycat",
                         "-E",
                         "-e",
                         "E",
                         "source/dir.tar.gz",
                         "/dev/null") ||
            call_system ("copycat",
                         "--extract-to-dir",
                         "--extract",
                         "extract-to-dir",
                         "source/dir.tgz",
                         "/dev/null") ||

            call_system ("copycat",
                         "-X",
                         "-e",
                         "E",
                         "source/dir.tar.gz",
                         "/dev/null") ||
            call_system ("copycat",
                         "--xml-dir",
                         "--extract",
                         "extract-to-dir",
                         "source/dir.tgz",
                         "/dev/null") ||

            call_system ("diff",
                         "source/dir/text.A",
                         "e/text.A") ||
            call_system ("diff",
                         "source/dir/text.A",
                         "extract/text.A") ||
            call_system ("diff",
                         "source/dir/text.A",
                         "E/dir.tar.gz/dir.tar/source/dir/text.A.bz2/text.A") ||
            call_system ("diff",
                         "source/dir/text.A",
                         "extract-to-dir/dir.tgz/dir.tar/source/dir/text.A.bz2/text.A") ||
            call_system ("diff",
                         "source/dir/text.B",
                         "e/text.B") ||
            call_system ("diff",
                         "source/dir/text.B",
                         "extract/text.B") ||
            call_system ("diff",
                         "source/dir/text.B",
                         "E/dir.tar.gz/dir.tar/source/dir/text.B.bz2/text.B") ||
            call_system ("diff",
                         "source/dir/text.B",
                         "extract-to-dir/dir.tgz/dir.tar/source/dir/text.B.bz2/text.B") );

    rmtree ("e") or die $! if ($error_count == 0);
    rmtree ("extract") or die $! if ($error_count == 0);
    rmtree ("E") or die $! if ($error_count == 0);
    rmtree ("extract-to-dir") or die $! if ($error_count == 0);
    rmtree ("source") or die $! if ($error_count == 0);
    rmtree ("destination") or die $! if ($error_count == 0);
}

# This is the actual main body
# Eventually this could be made selective instead of all or nothing.

cc_file_2_file;
cc_files_2_dir;
cc_files_2_devnull;
cc_files_2_stdout;
cc_gzip_file;
cc_bzip2_file;
cc_archive_file;
cc_nenc_file;
cc_extract;

exit ($error_count);

__END__

calling syntax:

copycat [options] src-file dst-file
copycat [options] src-file [src-file...] dst-dir
copycat [options] -o dst-dir src-file [src-file...]

copycat options:
  -x|--cache-dir <dir-path>        location of output cached files 
  -f|--force                       force overwrite of existing files 
  -o|--output <file-path>          location of output 
  -e|--extract <dir-path>          location of extracted files 
  -E|--extract-to-dir              extracted directories match normal XML 
  -X|--xml-dir                     XML matches extracted files 
  --xml-base-node <base-file-name> use this to base the XML not destination; 
                                   can only be used with single source 
source file known types:
    compression
        gzip [.gz]
        bzip [.bz2]
    archive
        tar  [.tar]
        kar  [.sra|.csra]- disabled
    source encryption
        'wga' [.ncbi_enc]
        ncbi  [.nenc]
    destination encryption
        ncbi  [.nenc]
    cached
        xml   [.xml]
    extracted
        all not archive (?)

------ Test cases
copycat source-file destination-file
copycat source-file destination-directory
copycat source-file /dev/null
copycat source-file /dev/stdout
copycat source-file /dev/fd/<N>
copycat source-file-1 source-file-2 destination-directory
copycat source-file-1 source-file-2 /dev/null


gzip/file       .XXX.gz
bzip/file       .XXX.bz2
tar/file[s]     {}.tar
kar/file[s]     {}.sra {}.csra {}.kar
nenc/file       .XXX.nenc
ncbi_enc/file   .XXX.ncbi_nenc
gzip/tar/file   {}.tar.gz {}.tgz


---
---
---
---
---
---
---
---
---
---

Not in  a test yet


copycat options:
  -x|--cache-dir <dir-path>        location of output cached files 
  -X|--xml-dir                     XML matches extracted files <<< tested but requires manual inspection
  --xml-base-node <base-file-name> use this to base the XML not destination; 
                                   can only be used with single source 
source file known types:
    source decryption
        'wga' [.ncbi_enc]
    cached
        xml   [.xml]

------ Test cases
copycat source-file /dev/fd/<N>
ncbi_enc/file   .XXX.ncbi_nenc






















































