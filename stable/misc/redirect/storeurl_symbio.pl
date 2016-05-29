#!/usr/bin/perl

use strict;
$|=1;
while (<>)
{

	if(/http:\/\/(\S+?)(\?\S+)?(\s+\S.*)/i)
	{
		my $path = $1;
		my $prot = $3;
		my $redi_ret =  "http://$path $prot\n";
		print $redi_ret;
		#`echo "rewrite: [http://$path $prot\]n" >>/tmp/download_redi.log`;
	}
	else
	{ 
		print $_; 
	}
}
