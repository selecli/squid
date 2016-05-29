#!/usr/bin/perl
$|=1;
while(<>)
{
	chomp;
	my @parts = split /####/,$_;
	my @url = split / /,$parts[0];
	my @accepted_header = split /	/,$parts[1];

	my $header;
	foreach my $a(@accepted_header){
		$header .= "  --header=\'$a\'  ";
	}
	
	my $status;
	if($url[0] =~ /^[\d]+/)
	{
  		$status = `wget $header -S -t 1 --dns-timeout=1 --connect-timeout=1 --read-timeout=5  --spider "$url[1]" 2>&1|grep "HTTP\/1\.[0|1]"`;
	}
	else
	{
		$status = `wget $header -S -t 1 --dns-timeout=1 --connect-timeout=1 --read-timeout=5  --spider "$url[0]" 2>&1|grep "HTTP\/1\.[0|1]"`;
	}
	chomp($status);
	print "httpstatus:$status\$\n";
}
