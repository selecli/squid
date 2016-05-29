#!/usr/bin/perl
use strict;

$|=1;

open LOG, ">>", "/tmp/storeurl.log";
while (<>)
{
        printf LOG "%s\n", $_;
        if (/(\d+) http:\/\/([^\?]+\?filepath=[^\&]+)&\S+(\s+.*)/i)
        {
                my $num = $1;
                my $path = $2;
                my $port = $3;
                my $ret = "$num http://$path$port\n";
                print $ret;
        }
        elsif (/http:\/\/([^\?]+\?filepath=[^\&]+)&\S+(\s+.*)/i)
        {
                my $path = $1;
                my $port = $2;
                my $ret = "http://$path$port\n";
                print $ret;
        }
	elsif ( /(\d+) http:\/\/(.*?)\?(.*)/ )
	{
		print "$1 http://" . $2 . "\n";
	}
	elsif ( /http:\/\/(.*?)\?(.*)/ )
	{
		print "http://" . $1 . "\n";
	}
        else
        {
                print $_;
        }
}
close(LOG);
