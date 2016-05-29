#!/usr/bin/perl
use strict;
$|=1;

#$_="http://cdn1.v.56.com/fcs71.56.com/flvdownload/0/23/133526481395hd_clear.flv";
#$_="http://cdn1.v.56.com/fcs71.56.com/flvout/0/23/133526481395hd_clear.flv";
#$_="http://f1.r.56.com/f1.c74.56.com/flvdownload/21/20/133686361750hd.flv?m=s&h=1145&h1=1229&start=3873449"
#$_="http://cp24.c110.play.bokecc.com/flvs/E0B63A54D48BA5B6/2012-06-01/79D95332AEE1F51D-1.flv?key=97421FB54803E156F708EA900AAE5EDA&upid=20914116061625021368612430456829"

while (<>)
{
	#if(/(\d+) http:\/\/([^\/]+)\/([^\s?]+)/i)
	if(/(\d+) http:\/\/(.*.56.com\/.*56.com)\/([^\s?]+)/i)
	{
#`echo "1--- $_">>/tmp/1.log`;
		my $first = $1;
		my $host = $2;
		my $url = $3;
		my $redi_ret =  "$first http://$host/$url\n";
		$redi_ret =~ s/flvdownload/flvout/;
		print $redi_ret;

#`echo "2--- $redi_ret">>/tmp/1.log`;
	}
	elsif(/(\d+) http:\/\/(.*.bokecc.com)\/([^\s?]+)/i)
	{
#`echo "1--- $_">>/tmp/1.log`;
                my $first = $1;
                my $host = $2;
                my $url = $3;
                my $redi_ret =  "$first http://$host/$url\n";
                print $redi_ret;
#`echo "2--- $redi_ret">>/tmp/1.log`;
 
	}
	else
	{
		print $_;
	}
}

