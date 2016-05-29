#!/usr/bin/perl

# youku tudou 6cn cctv uusee kumi storeurl
use strict;
$|=1;
while (<>)
{
        if(/(\d+) http:\/\/ccflvcache\.yk\.ccgslb\.net\/[^\/]+\/youku\/[^\/]+(\S+\?preview_num=30&preview_ts=)+\S+\s+(\S.*)/i)
        {
                my $num = $1;
                my $filename = $2;
                my $port = $3;
                my $redi_ret =  "$num http://youku$filename $port\n";
                print $redi_ret;
        }
        elsif(/(\d+) http:\/\/ccflvcache\.yk\.ccgslb\.net\/[^\/]+\/youku\/[^\/]+\/(\S+?)(\?\S+)?(\s+\S.*)/i)
        {
                my $num = $1;
                my $filename = $2;
		my $prot = $4;
                my $redi_ret =  "$num http://youku/$filename$prot\n";
                print $redi_ret;
        }
        elsif(/(\d+) http:\/\/ccflvcache\.ccgslb\.net\/([^\/]+\/\S.*)\?\S+&key=\S+&itemid+\S+(\s+\S.*)/)
        {
		my $num = $1;
                my $path = $2;
                my $port = $3;
                my $redi_ret = "$num http://$path$port\n";
                print $redi_ret;
       }
        elsif(/(\d+) http:\/\/cpis\.6room\.ccgslb\.net\/[^\/]+\/(\S+?)(\?.*)?key1=+\S+(\s+\S.*)/)
        {
		my $num = $1;
                my $path = $2;
                my $port = $4;
                my $redi_ret = "$num http://$path$port\n";
                print $redi_ret;
        }
        elsif(/(\d+) http:\/\/cpis\.cctv\.ccgslb\.net\/[^\/]+\/data\d+\/(.*\/Zmxhc2g-\/\S.*?)(\?\S+)?(\s+\S.*)/)
        {
		my $num = $1;
                my $path = $2;
                my $port = $4;
                my $redi_ret = "$num http://$path$port\n";
                print $redi_ret;
        }
        elsif(/(\d+) http:\/\/cpis\.sina\.ccgslb\.net\/[^\/]+\/(\S+\.hlv?)(\?vstr=\S+pid=\S+)?(\s+\S.*)/)
        {
                my $num = $1;
                my $path = $2;
                my $port = $4;
                my $redi_ret = "0 http://$path$port\n";
                print $redi_ret;
        }
	elsif(/(\d+) http:\/\/cpis\.ku6\.ccgslb\.net\/[^\/]+\/(\S.*?)(\?\S+id=ku6_vod\S+)?(\s+\S.*)/)
        {
                my $num = $1;
                my $path = $2;
                my $port = $4;
                my $redi_ret = "0 http://$path$port\n";
                print $redi_ret;
        }
	elsif(/(\d+) http:\/\/(v1\.uusee\.com\/\S+?)(\?\S+)?(\s+\S.*)/)
	{
		my $num = $1;
		my $path = $2;
		my $port = $4;
		my $redi_ret = "$num http://$path$port\n";
                print $redi_ret;
	}
	elsif(/(\d+) http:\/\/(video\.uusee\.ccgslb\.net\/\S+?)(\?\S+)?(\s+\S.*)/)
        {
                my $num = $1;
                my $path = $2;
                my $port = $4;
                my $redi_ret = "$num http://$path$port\n";
                print $redi_ret;
        }
	elsif(/(\d+) http:\/\/(v\.kumi\.cn\/\S+?)(\?\S+)?(\s+\S.*)/)
        {
                my $num = $1;
                my $path = $2;
                my $port = $4;
                my $redi_ret = "$num http://$path$port\n";
                print $redi_ret;
        }
	elsif(/(\d+) (http:\/\/.*\/)(sohu\/\S+)(\?\S+)+\S+\s+(\S.*)/i)
	{   
		my $num = $1; 
		my $filename = $3; 
		my $port = $5; 
		my $redi_ret =  "$num http://$filename $port\n";
		print $redi_ret;
	}
	elsif(/(\d+) http:\/\/[^\/]+:8080\/(.*\.mp4)(\S+?)(\?\S+)?(\s+\S.*)/i)
        {
                my $num = $1;
                my $filename = $2;
                my $port = $5;
                my $redi_ret =  "$num http://$filename $port\n";
                print $redi_ret;
        }
	elsif(/(\d+) http:\/\/[^\/]+\/(\S+?)(\?crypt=\S+)?(\s+\S.*)/i)
	{
		my $num = $1;
		my $filename = $2;
		my $port = $4;
		my $redi_ret =  "$num http://$filename $port\n";
		print $redi_ret;
	}
	else
	{ 
		print $_;
	}
}
