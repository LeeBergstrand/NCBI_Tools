use File::Basename;
use POSIX qw(strftime);

my %ERROR;
my $APP = basename($0);

sub GetAppName { $APP }

sub XmlSafe { ($_) = @_; s/&/&amp;/g; s/'/&quot;/g; s/"/&quot;/g; s/</&lt;/g; $_ }
sub Timestamp { "timestamp=\"" . POSIX::strftime("%Y-%m-%dT%H:%M:%S", gmtime) . "\"" }

sub LogExec { my ($cmd, $noClose) = @_;
    print "<Exec cmd=\"" . XmlSafe($cmd) . "\" " . Timestamp . " app=\"$APP\" pid=\"$$\""; print "/" unless ($noClose); print ">\n"; }
sub LogCheck { my ($file) = @_; print "<Check file=\"$file\" " . Timestamp . " app=\"$APP\" pid=\"$$\"/>\n"; }
sub LogSql   { my ($sql ) = @_; print "<Db sql=\"" . XmlSafe($sql) . "\" " . Timestamp . " app=\"$APP\" pid=\"$$\"/>\n"; }
sub Log { my ($node, $msg, $priv) = @_; print "<$node message=\""  . XmlSafe($msg) . "\" " . Timestamp . " app=\"$APP\" pid=\"$$\"";
    print " reason=\"$priv\"" if ($priv); print "/>\n"; }
sub LogError { my ($msg, $priv) = @_; Log "Error", $msg, $priv; }

sub Error {
    my ($msg, $priv) = @_;
    LogError($msg, $priv);
    $ERROR{msg} = $msg;
}

sub HasError { %ERROR }

sub Chdir { my ($dir) = @_; print "<Chdir dir=\"$dir\" " . Timestamp . " app=\"$APP\" pid=\"$$\"/>\n"; chdir $dir || Error "cannot cd $dir"; }

sub NeedFixXml { ($_) = @_; /[\x80-\xfc]/; }

sub FixXml {
    my ($name, $bak);
    ($name, $bak, $_) = @_;
    s/[\x80-\xfc]/ /g;
    rename($name, $bak) || Error("cannot mv \"$name\" \"$bak\": $!");
    unless (HasError())
    {   open(OUT, ">$name") || Error("cannot open $name to write: $!"); }
    unless (HasError()) {
        print OUT $_;
        close OUT || Error("cannot close $name: $!");
    }
    $_;
}

sub SetEnv {
    my $hostname = `hostname`;
    chomp $hostname;
    my $sandbox = '';
    $sandbox = 'sand' if ($hostname =~ /^wgasra0[0-9]$/);
    my $TRACE_SOFTWARE = "/panfs/${sandbox}traces01.be-md.ncbi.nlm.nih.gov/trace_software";
    my $VDB_ROOT = "$TRACE_SOFTWARE/vdb";
    my $SRA_CENTOS = "$TRACE_SOFTWARE/toolkit/centos64";
    my $SRA_TOOLS = "$SRA_CENTOS/bin";
    my $SRA_LIBS = "$SRA_CENTOS/lib";
    my $LD_LIBRARY_PATH = $ENV{LD_LIBRARY_PATH};
    $ENV{LD_LIBRARY_PATH} = "$SRA_LIBS:$LD_LIBRARY_PATH";
    $ENV{VDB_ROOT} = "$VDB_ROOT";
}

1
