#!/usr/bin/perl
use strict;
$|=1;

#$_="http://cdn1.v.56.com/fcs71.56.com/flvdownload/0/23/133526481395hd_clear.flv";
#$_="http://cdn1.v.56.com/fcs71.56.com/flvout/0/23/133526481395hd_clear.flv";


while (<>)
{
	if(/(\d+) http:\/\/([^\/]+)\/([^\s?]+)/i)
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
	else
	{
		print $_;
	}
}

