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
use IO::File;
use File::Basename;

sub usage()
{
    print "Usage:\n";
    print "  sam-tools-dump.pl <BAM-FILE> <OUT-DIR> [-DEBUG]\n\n";
    print "Summary:\n";
    print "  Create a set of direftories and files from the data in a\n";
    print "  BAM file so it can be compared against data similarly created\n";
    print "  from the database created bu bam-load.\n\n";
    print "Parameters:\n";
    print "  <BAM-FILE> is the path to a BAM file that will be processed by\n";
    print "             the external samtools utility.\n";
    print "  <OUT_DIR>  is the directory in which to create our data files\n";
    print "Options:\n";
    print "  -CS        use color space optional columns instead of base space\n";
    print "             required columns.  This program fails if these optional\n";
    print "             columns are not present in the BAM file.\n";
    print "  -DEBUG     this will just display some messages showing progess\n";
    print "             as the script runs.\n\n";
}


sub xform_qual($)
{
    local $_=shift;
    # N < 33+10 -> '"'
    # N < 33+20 -> '+'
    # N < 33+30 -> '5'
    # N >= 33+30 -> '?'
    return ($_ lt '+') ? '"' : (($_ lt '5') ? '+' : (($_ lt '?') ? '5' : '?'));
}


# options and parameters
my $debug = 0;      # default off
my $colorspace = 0; # default off
my $infile;
my $outdir;
my $index;
my @params;
my $samtools;
my $tmpfs;
my $log;


sub log_it
{
    my $now = localtime();
    my $fmt = shift;
    printf ("%s: $fmt",$now,@_) if ($debug);
    printf{$log} ("%s: $fmt",$now,@_);
}


BEGIN
{
    $tmpfs = "/export/home/TMP" if -d "/export/home/TMP";
    $tmpfs = "/tmp" unless $tmpfs;
    $samtools = `which samtools 2>/dev/null` || "$ENV{NCBI}/samtools/bin/samtools";
    chomp $samtools;
}

for ($index = 0; $index < $#ARGV + 1; ++$index)
{
    if (($ARGV[$index] eq "-debug") || ($ARGV[$index] eq "-DEBUG"))
    {
        $debug = 1;
    }
    elsif (($ARGV[$index] eq "-cs") || ($ARGV[$index] eq "-CS"))
    {
        $colorspace = 1;
    }
    else {
        push(@params, $ARGV[$index]);
    }
}


if ($#params < 1)
{
    print ("too few parameters\n");
    usage();
    exit(3);
}
elsif ($#params > 1)
{
    print ("too many parameters\n");
    usage();
    exit(3);
}
$infile = $params[0];
$outdir = $params[1];

# start logging
open ($log, ">", "$outdir/sam-tools-dump.$$.log") or die "Cannot open log for output";
log_it ">>> sam-tools-dump started for $infile to $outdir <<<\n" if ($debug);

# the 11 mandatory fields in a SAM/BAM file record and some we pull out the optional fields or create
my @columns = ('QNAME','FLAG','RNAME','POS','MAPQ','CIGAR','RNEXT','PNEXT','TLEN','SEQ','QUAL','RG','RC','R','ROW_ID');

# add columns to these to write them in to the table directories in the respective aligned/unaligned
# meta table directories - the order in this array is the primary key for ordering first and then on down
#my @aligned_columns = ( 'POS','SEQ','QUAL','MAPQ','TLEN','RNEXT','PNEXT','RC','R' );
my @aligned_core_columns = ( 'POS', 'SEQ', 'QUAL', 'MAPQ' );
my @aligned_ext_columns = ( 'POS', 'TLEN', 'RNEXT', 'PNEXT' );
my @unaligned_columns = ( 'SEQ', 'QUAL' );

# bit field flags for FLAGS column
my $flag_template = 1;
my $flag_aligned = 2;
my $flag_unmapped = 4;
my $flag_next_unmapped = 8;
my $flag_reverse_complemented = 16;
my $flag_reverse = 32;
my $flag_first_frag = 64;
my $flag_last_frag = 128;
my $flag_secondary_alignment = 256;
my $flag_no_quality = 512;
my $pcr_dup = 1024;

# open the BAM file parameter pulling the SAM file version out through external utility samtools
# samtools will turn the BAM file into a SAM stream
open (SAMIN, "-|", "$samtools view $infile") or die "open sam from bam failed";

log_it ("sam-tools-dump started for $infile to $outdir\n");

# create the sam tools subdirectory of the current working directory
# and change to it
mkdir "$outdir"; # fail is okay if it already exists.
mkdir "$outdir/samtools";

# make the directories for unaligned versus unaligned records
mkdir "$outdir/samtools/unaligned";
mkdir "$outdir/samtools/aligned";

my $tmpdir = "$tmpfs/sam-tools-dump.$$";

mkdir $tmpdir or die $!;

# make the directories for unaligned versus unaligned records
mkdir "$tmpdir/samtools";
mkdir "$tmpdir/samtools/unaligned";
mkdir "$tmpdir/samtools/aligned";

# create hash for the rname map into the aligned records
my %rnames;

# create hash for the spotgroup map into the unaligned records
my %spotgroups;

# now loop through the lines of the SAM file
my $samrow;
my $row_id = 1;
while (($samrow = <SAMIN>), defined($samrow))
{

    # eat CR/LF or whatever EOL we got
    chomp ($samrow);

    # split the row on the TAB boundaries
    my @fields = split /\t/,$samrow;

    # hash for column name/value pairs
    my %col = ();

    # tuck the required columns into the col hash
    for ($index = 0; $index < 11; $index++)
    {
        $col{$columns[$index]} = $fields[$index];
    }

    $col{'RC'} = $col{'FLAG'} & $flag_reverse_complemented ? '1' : '0';
    $col{'R'} = $col{'FLAG'} & $flag_reverse ? '1' : '0';

    # now scan for the optional fields we support/need
    # currently RG is spotname and OQ is original quality
    # original quality supercedes 'QUAL'
    my $found_cs = 0;
    my $found_cq = 0;
    for (; $index <= $#fields; ++$index)
    {

        my @opt = split(/:/,$fields[$index]);

        if ($opt[0] eq 'RG')
        {

            $col{'RG'} = $opt[2];
        }
        elsif ($opt[0] eq 'OQ')
        {
            # colon is a legal character in the OQ value string
            # so we are a bit more complicated
            #$col{'QUAL'} = $opt[2];
            $col{'QUAL'} = substr ($fields[$index],5);
        }
        elsif (($opt[0] eq 'CQ') && ($colorspace))
        {
            $found_cq = 1;
            #$col{'QUAL'} = $opt[2];
            $col{'QUAL'} = substr ($fields[$index],5);
        }
        elsif (($opt[0] eq 'CS') && ($colorspace)) {
            $found_cs = 1;
            $col{'SEQ'} = $opt[2];
        }
    }

    if (($colorspace) && (!found_cs) && (!$found_cq)) {
        log_it ("requested colorspace but did not find both CS and CQ row '$row_id'\n");
        exit (3);
    }

    # if there is no optional column 'RG' use 'def'
    $col{'RG'} = "def" unless (exists $col{'RG'});

    # run our transform on the quality row
    $col{'QUAL'}=~s/(.)/xform_qual($1)/eg if (exists $col{'QUAL'});

    # create a row-id column
    $col{'ROW_ID'}=$row_id;

# debug output
#  if ($debug) {
#    foreach(@columns) {
#      print "$_\t$col{$_}\n";
#    }
#    print("\n");
#  }
#end debug output

    # do we have a FLAG field?
    if (defined$col{'FLAG'})
    {
        # is the unmapped bit set?
        if($col{'FLAG'} & $flag_unmapped)
        {
#            log_it "unmapped\n" if ($debug);

            # we put the columns in a table directory based on spotgroup
            my $spotgroup = $col{'RG'};

            #if we haven't already hit this table we must create it

            if(!exists($spotgroups{$spotgroup}))
            {
                log_it("new table unaligned/$spotgroup\n");
                $spotgroups{$spotgroup} = new IO::File "$tmpdir/samtools/unaligned/$spotgroup", "w" or die  $!;
            }

            # using for each gets too many tabs
            for ($index = 0; $index < $#unaligned_columns; ++$index)
            {
                $spotgroups{$spotgroup}->print("$col{$unaligned_columns[$index]}\t");
            }
            $spotgroups{$spotgroup}->print("$col{$unaligned_columns[$index]}\n");

        }
        else
        {
#            log_it ( "aligned\n" ) if ($debug);

            # we put the columns in a table directory based on rname
            my $pos = int( $col{'POS'} / 10000000 );
            my $f_core_name = "core_" . $col{'RNAME'} . "_" . $pos;
            my $f_ext_name = "ext_" . $col{'RNAME'} . "_" . $pos;

            if ( $col{'TLEN'} == 0 )
            {
                $col{'RNEXT'} = '*';
                $col{'PNEXT'} = '0';
                $col{'R'} = '0';
            }

            #if we haven't already hit this table we must create it

            if(!exists($rnames{$f_core_name}))
            {
                log_it("new table aligned/$f_core_name\n");
                $rnames{$f_core_name} = new IO::File "$tmpdir/samtools/aligned/$f_core_name", "w" or die $!;
            }
            if(!exists($rnames{$f_ext_name}))
            {
                log_it("new table aligned/$f_ext_name\n");
                $rnames{$f_ext_name} = new IO::File "$tmpdir/samtools/aligned/$f_ext_name", "w" or die $!;
            }

            # using for each gets too many tabs
            for ($index = 0; $index < $#aligned_core_columns; ++$index)
            {
                $rnames{$f_core_name}->print("$col{$aligned_core_columns[$index]}\t");
            }
            $rnames{$f_core_name}->print("$col{$aligned_core_columns[$index]}\n");

            for ($index = 0; $index < $#aligned_ext_columns; ++$index)
            {
                $rnames{$f_ext_name}->print("$col{$aligned_ext_columns[$index]}\t");
            }
            $rnames{$f_ext_name}->print("$col{$aligned_ext_columns[$index]}\n");

        }
    }
    ++$row_id;
}
log_it("finished reading $infile\n");
#close SAM file
close SAMIN;

my $spotgroupsize = scalar keys %spotgroups;
if ($spotgroupsize >= 0)
{
    foreach (keys %spotgroups)
    {
        $spotgroups{$_}->close();
    }
}

my $rnamessize = scalar keys %rnames;
if ($rnamessize >= 0)
{
    foreach (keys %rnames)
    {
        $rnames{$_}->close();
    }
}

# we have to sort each file.  We will run the sorts in parallel.
my %sortpids;
my $cpid;
log_it("starting sorting\n");

my @files = <$tmpdir/samtools/aligned/*>;
my @otherfiles = <$tmpdir/samtools/unaligned/*>;
push (@files, @otherfiles);

foreach $file (@files)
{
    #print ("$file ".keys(%sortpids)."\n");
    if (keys(%sortpids) == 4)
    {
        $cpid = waitpid (-1, 0);
        print "Finished $cpid $sortpids{$cpid}\n" if ($debug);
        log_it ("Finished $cpid $sortpids{$cpid}\n");
        delete $sortpids{"$cpid"};
    }
    my $file = substr ($file,length("$tmpdir/"));
    $cpid = fork();
    if ($cpid == 0) {
        my $basename;
        my $path;
        my $suffix;
        ($basename,$path,$suffix) = fileparse ($file);
#        my $basename = basename($file);
        log_it ("Child sort of $tmpdir/$file to $outdir/$file $basename\n");
        if (substr($basename,0,4) eq "ext_")
        {
            exec ("sort","-t\t","-n","-k1","-k2","$tmpdir/$file","--output=$outdir/$file");
        }
        else
        {
            exec ("sort $tmpdir/$file --output=$outdir/$file");
        }
    }
    else
    {
        log_it "Starting sort $file\n";
        $sortpids{"$cpid"} = $file;
    }
}

while (keys (%sortpids))
{
    $cpid = waitpid (-1, 0);
    log_it"Finished $cpid $sortpids{$cpid}\n";
    delete $sortpids{"$cpid"} or die $!;
}

# clear TMP directory
system ("rm","-fr","$tmpdir");

log_it (">>> sam-tools-dump finished for $infile to $outdir <<<\n");
close ($log);
__END__
