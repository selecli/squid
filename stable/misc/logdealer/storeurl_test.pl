#!/usr/bin/perl

use strict;
$|=1;
while (<>)
{       
	`echo "first $_\n" >>/tmp/redi.log`;
#	echo "1 http://live.cnlive.com:1935/liveorigin/news/media_12407.ts?wowzasessionid=294867743"
	if(/(\d+) (http:\/\/.*\.ts)\?wowzasessionid=.*/i)
	{
		my $first = $1;
		my $second = $2;
		my $redi_ret =  "$first $second\n";
		print $redi_ret;
	}

        elsif(/(\d+) (http:\/\/qiangsheng-http\.ccgslb\.com\.cn\/.*\.mp4).*/i)
        {
                my $num = $1;
                my $path = $2;
                my $ret = "$num $path\n";

                print $ret;
        # `echo "2--------- $ret" >>/tmp/redi.log`;
        }

        elsif(/(\d+) (http:\/\/cdn-ip-hv\.vtxcloud\.com\/.*\.(mp4|flv)).*/i)
        {
                my $num = $1;
                my $path = $2;
                my $ret = "$num $path\n";

                print $ret;
        #  `echo "2--------- $ret" >>/tmp/redi.log`;
        }
		elsif(/(\d+) http:\/\/cdnv\.etiantian\.com\/(\w+)\/(\w+)\/(.*\.mp4|flv|f4v).*/i)
		{
			my $ret = "$1 http://cdn.etiantian.com/$4\n";
			print $ret;
		}
	else
	{
		print $_;
	}
}
