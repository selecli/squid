#!/usr/bin/perl

use strict;
$|=1;
while (<>)
{       
#`echo "0--- $_">>/tmp/1.log`;
#        if(/(\d+) http:\/\/[^\/]+\/(\S+?)?(\s+\S.*)/i)
	if(/(\d+) http:\/\/([^\/]+)\/([^\s?]+)/i)
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
