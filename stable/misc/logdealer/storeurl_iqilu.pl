#!/usr/bin/perl

use strict;
$|=1;
while (<>)
{

        #`echo "0---- $_\n" >>/tmp/download_redi.log`;
        if(/(\d+) http:\/\/[^\/]+\/[^\/]+\/[^\/]+\/[^\/]+\/(\S+?)(\?\S+)?(\s+\S.*)/i)
        {
          #      `echo "1---- $_\n" >>/tmp/download_redi.log`;
                my $first = $1;
                my $path = $2;
                my $prot = $4;
                my $redi_ret =  "$first http://$path$prot\n";
                print $redi_ret;
           #     `echo "2---- $redi_ret\n" >>/tmp/download_redi.log`;
        }
        else
        { 
		print $_; 
	}
}
