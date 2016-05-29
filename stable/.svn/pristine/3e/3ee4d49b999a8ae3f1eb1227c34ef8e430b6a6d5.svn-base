#!/usr/bin/perl

use strict;
$|=1;
while (<>)
{       

        if(/(http:\/\/)[^\/]+\/([^\/]+)\/[^\/]+(\S+\?preview_num=30&preview_ts=)+\S+\s+(\S.*)/i)
	{
                my $first = $1;
                my $path = $2;
                my $filename = $3;
		my $port = $4;
                my $redi_ret =  "0 $first$path$filename $port\n";
                print $redi_ret;
	}
        elsif(/(http:\/\/)[^\/]+\/([^\/]+)\/[^\/]+(\S.*)/i)
        {      
                my $first = $1;
                my $path = $2;
                my $filename = $3;
                my $redi_ret =  "0 $first$path$filename\n";
                print $redi_ret;
        }
        else
        {
       		print $_;
	}
}
