#!/usr/bin/perl
$| = 1;

while (<>)
{
	my $line = $_;
#	if($line =~ /http:\/\/([^\/]+)\/(\S+)/)
	if($line =~ /http:\/\/([^\/]+)\/(\S+)@@@(\d+)/)
	{       
		my $host = $1;
		my $uri = $2;
		my $port = $3;
		#my $result = `curl -svo /dev/null "http://127.0.0.1:$3/$uri" -H Host:$1 -H "X-CC-Media-Prefetch:1" --max-time 120 --connect-timeout 10 2>&1|grep "HTTP/1."`;
		my $result = `curl -svo /dev/null -x 127.0.0.1:$3 "http://$host/$uri" -H "X-CC-Media-Prefetch:1" --max-time 120 --connect-timeout 10 2>&1|grep "HTTP/1."`;
		if($result =~ /200.*OK/)
		{
			print "OK\n";
		}
		else
		{
			print "ERR\n";

		}
	}
	else    
	{       print "ERR\n";
	}
}
