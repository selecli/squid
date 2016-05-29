#!/usr/bin/perl
$| = 1;
#    input url
#    http://www.194.com/ts/1.txt CC_WASU_PREFETCH_TS http://www.194.com/ts/ad.txt

while (<>)
{
	my $line = $_;
	my $ad_len = -1,$normal_len = -1;
	#if($line =~ /http:\/\/([^\/]+)\/([^\s?]+)/)
	if($line =~ /http:\/\/([^\/]+)\/([^\s?]+) CC_WASU_PREFETCH_TS http:\/\/([^\/]+)\/([^\s?]+)/)#normal & ad
	{
		my $host_normal = $1;
		my $uri_normal = $2;
		my $host_ad = $3;
		my $uri_ad = $4;
		my $ret_normal=`curl -I "http://127.0.0.1/$uri_normal" -H Host:$host_normal --max-time 10 --connect-timeout 10 2>&1`;
		my $ret_ad=`curl -I "http://127.0.0.1/$uri_ad" -H Host:$host_ad --max-time 10 --connect-timeout 10 2>&1`;
		if($ret_ad =~ /200.*OK/)
		{
			if ($ret_ad =~ /Content-Length: (\d+)/)
			{
				$ad_len = $1;
			}
		}
		if($ret_normal =~ /200.*OK/)
		{
			if ($ret_normal =~ /Content-Length: (\d+)/)
			{
				$normal_len = $1;
			}
		}
		
		if ($ad_len >=0 && $normal_len >=0)
		{
			print "ad_len=$ad_len normal_len=$normal_len\n";
		}
		else
		{
			print "prefetch failed,check the file[http://$host_normal/$uri_normal and http://$host_ad/$uri_ad] exists?!\n";
		}
	}
	else
	{       print "helper submit url format mismatching! check the mod_merger_request helperSubmit buf!\n";
	}
}
