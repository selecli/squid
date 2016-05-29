#!/usr/bin/perl

# youku tudou 6cn cctv uusee kumi storeurl
use strict;
$|=1;
while (<>)
{
`echo "$_">>/tmp/all.log`;
        if(/(\d+) http:\/\/[^\/]+\/youku\/[^\/]+(\S+\?preview_num=30&preview_ts=)+\S+\s+(\S.*)/i)
        {
                my $num = $1;
                my $filename = $2;
                my $port = $3;
                my $redi_ret =  "$num http://youku$filename $port\n";
                print $redi_ret;
        }
        elsif(/(\d+) http:\/\/[^\/]+\/youku\/[^\/]+\/(\S+?)(\?\S+)?(\s+\S.*)/i)
        {
                my $num = $1;
                my $filename = $2;
		my $prot = $4;
                my $redi_ret =  "$num http://youku/$filename$prot\n";
                print $redi_ret;
        }
        elsif(/(\d+) http:\/\/[^\/]+\/(\S.*)\?\S+&key=\S+&itemid+\S+(\s+\S.*)/)
        {
		my $num = $1;
                my $path = $2;
                my $port = $3;
                my $redi_ret = "$num http://$path$port\n";
                print $redi_ret;
        }
        elsif(/(\d+) http:\/\/[^\/]+\/(\S+?)(\?.*)?key1=+\S+(\s+\S.*)/)
        {
		my $num = $1;
                my $path = $2;
                my $port = $4;
                my $redi_ret = "$num http://$path$port\n";
                print $redi_ret;
        }
        elsif(/(\d+) http:\/\/[^\/]+\/data\d+\/(.*\/Zmxhc2g-\/\S.*?)(\?\S+)?(\s+\S.*)/)
        {
		my $num = $1;
                my $path = $2;
                my $port = $4;
                my $redi_ret = "$num http://$path$port\n";
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
	elsif(/(\d+) http:\/\/(dlied[1-6]\.qq\.com\/\S+?)(\?\S+)?(\s+\S.*)/)
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
#`echo "2--- $redi_ret">>/tmp/pptv.log`;
        }
	elsif(/(\d+) http:\/\/[^\/]+\/(\S+?)(\?crypt=\S+)?(\s+\S.*)/i)
	{
		my $num = $1;
		my $filename = $2;
		my $port = $4;
		my $redi_ret =  "$num http://$filename $port\n";
		print $redi_ret;
#		`echo "$redi_ret">>/tmp/letv.log`;
	}
	else
	{ 
		print $_;
	}
}
