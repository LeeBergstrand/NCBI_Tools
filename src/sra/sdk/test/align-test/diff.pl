#!/usr/bin/perl -w

my $samtools;
my $lead_cmd;

BEGIN {
    chomp($samtools = `which samtools`);
    die 'need samtools' unless $samtools;

    chomp($lead_cmd = `which samview`);
    die 'need samview' unless $lead_cmd;
}

my $gold_cmd = "$samtools view";
my $bam = shift or die "need a source file";
my @refs;

{
	my %u;
	
	%u = map { ($_, 1) } map {
            (split(/:\s*/, $_))[1]
        } map {
            grep { /^SN:.+/ } split("\t", $_)
        } grep { /^\@SQ\s/ } split("\n", `$gold_cmd -H $bam`);

	@refs = sort keys %u;
}

sub compare($) {
	my $ref = shift | "";
	my $same = 0;
	my $fail = 0;
	
	open GOLD, "-|", "$gold_cmd $bam $ref" or die "failed to open gold";
	open LEAD, "-|", "$lead_cmd $bam $ref" or die "failed to open lead";
	
	my $gold_lines = 0;
	my $lead_lines = 0;
	
	for ( ; ; ) {
		my $gold = <GOLD>;
		my $lead = <LEAD>;
		
		++$gold_lines if defined $gold;
		++$lead_lines if defined $lead;
		
		last unless $gold;
		last unless $lead;
		
		chomp $gold;
		chomp $lead;
		
		if ($gold ne $lead) {
			++$fail;
			if ($fail <= 100) {
				print "+$gold_lines:$gold\n";
				print "------\n";
				print "-$lead_lines:$lead\n\n";
				return;
			}
			elsif ($fail == 101) {
				print "too many ...\n";
			}
		}
		else {
			++$same;
		}
	}
	close GOLD;
	close LEAD;
	
	if ($fail == 0 && $gold_lines != $lead_lines) {
		print "line count mismatch: gold has $gold_lines, lead has $lead_lines\n";
	}
	print 'Comparison for '.(length($ref) ? $ref : 'all').": $same records matched, $fail records did not match.\n";
}

print "Comparing '$bam':\n";
compare($_) foreach (@refs);
compare("");
