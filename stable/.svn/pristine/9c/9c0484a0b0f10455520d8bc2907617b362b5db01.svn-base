#!/usr/bin/perl
use LWP::Simple qw(get);
use Digest::MD5 qw(md5_hex);
$| = 1;

open my $fh, ">>", "/data/proclog/log/squid/query_check.log" or die "Open log file Error: $!";

open my $snfh, "/sn.txt" or die "Open sn file Error: $!";

my $sn = <$snfh>;
chomp($sn);
close $snfh;

while (my $line = <>)
{
	chomp $line;

#	if($line eq "[null_entry]" || $line eq "[null_mem_obj]")
#	{
#		print "ERR\n";
#		next;
#	}

	my $md5 = md5_hex($line);

#	print $fh "[$line]->[$md5]\n";	

	my $result = get("http://qsp.chinacache.com/stc.php?query=$md5&sn=$sn");

#	print $fh "[$line]->[$result]\n";
	
	if($result =~ /^\d+\.\d+\.\d+\.\d+.*$/)
	{
		print "$result\n";
	}
	else
	{
		print "ERR\n";

	}
}

close $fh;
