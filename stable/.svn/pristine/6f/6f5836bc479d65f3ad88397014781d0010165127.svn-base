#!/usr/bin/perl

use strict;

$|=1;

while(<>)
{
		if(/\.aipai\.com/i)
		{
				if (/^(\d+) \S+:\/\/(\S+)\?measure=1(\s+\S+\s+.*)/i)
				{
						my $first = $1;
						my $uri = $2;
						my $port = $3;
						my $redi_ret = "$first http://$uri$port\n";
						print $redi_ret;
				}
				elsif (/^(\d+) \S+:\/\/(\S+\?)measure=1&(\S+)(\s+\S+\s+.*)/i)
				{
						my $first = $1;
						my $uri = $2 . $3;
						my $port = $4;
						my $redi_ret = "$first http://$uri$port\n";
						print $redi_ret;
				}
				else
				{
						print $_;
				}
		}
		#elsif (/http:\/\/\.*\.aipai\.com\/\S+(&start=[0-9]{1,})\S+/i)
		#{
		#		if (/(\d+) http:\/\/(.*)/i)
		#		{
		#				my $first = $2;
		#				my $uri = $3;
		#				my $redi_ret = "$first http://$uri$1\n";
		#				print $redi_ret;
		#		}
		#		else
		#		{
		#				print $_;
		#		}
		#}
		else
		{
				print $_;
		}
}
