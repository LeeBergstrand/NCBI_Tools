#!/usr/bin/perl -w
use strict;
use tracedb;

my $path = '/net/nasan13/vol/trace3/STraceData_001';
my $ti = 1;#217108;

my @numbers = map { $_ < 10 ? " $_ " : "$_ " } (0..99);

my $db = tracedb::Create;
die "error creating tracedb!\n" if !$db;

my ($error,$data) = tracedb::GetTraceData($db, $path, $ti);

if( ! $error )
{
  my $basecall = tracedb::GetBasecall( $data);
  print "basecall:$basecall\n";
  my $qualscore = tracedb::GetQualscore( $data);
  $qualscore =~ s/(.)/$numbers[ord $1]/sg;
  $qualscore =~ s/(.{59})./$1\n/g;
  $qualscore =~ s/[\n ]$//;
  print "qualscore:\n", $qualscore, "\n";

  tracedb::DestroyTraceData( $data);
}
elsif( $error > 0 )
{
  warn "Error $error reading trace data!\n";  
}
else
{
  warn "Error reading trace data!\n";
}

tracedb::Destroy( $db);
