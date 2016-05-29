#!/usr/bin/perl

use strict;
$|=1;
while (<>)
{

        if(/http:\/\/(tj.*\.dhot\.v\.iask\.com\/\S.*)/)
        {
#`echo "first $_\n" >>/tmp/sina_redi.log`;
                my $path = $1;
                my $port = $2;
                my $redi_ret = "0 http://$path$port\n";
                print $redi_ret;
#`echo "second $redi_ret\n" >>/tmp/sina_redi.log`;
        }
        elsif(/http:\/\/[^\/]+\/(\S.*)\?\S+&key=\S+&itemid+\S+(\s+\S.*)/)
        {
#`echo "first $_\n" >>/tmp/tudou_redi.log`;
                my $path = $1;
                my $port = $2;
                my $redi_ret = "0 http://$path$port\n";
                print $redi_ret;
#`echo "second $redi_ret\n" >>/tmp/tudou_redi.log`;
        }
        #elsif(/http:\/\/[^\/]+\/(\S.*)\?\S+key1=\S+&key2=\S+&key3=\S+&key4=\S+(\s+\S.*)/)
        elsif(/http:\/\/[^\/]+\/(\S.*\?.*)key1=+\S+(\s+\S.*)/)
        {
#`echo "first $_\n" >>/tmp/6room_redi.log`;
                my $path = $1;
                my $port = $2;
                my $redi_ret = "0 http://$path$port\n";
                print $redi_ret;
#`echo "second $redi_ret\n" >>/tmp/6room_redi.log`;

        }
        elsif(/http:\/\/[^\/]+\/([^\/]+\/flvdownload\/\S.*)\?\S+(\s+\S.*)/)
        {
#`echo "first $_\n" >>/tmp/56_redi.log`;
                my $path = $1;
                my $port = $2;
                my $redi_ret = "0 http://$path$port\n";
                print $redi_ret;
#`echo "second $redi_ret\n" >>/tmp/56_redi.log`;
        }
        #elsif(/http:\/\/[^\/]+\/.*\/Zmxhc2g-\/(\S.*)+(\s+\S.*)/)
        elsif(/http:\/\/[^\/]+\/(.*\/Zmxhc2g-\/\S.*)+(\s+\S.*)/)
        {
#`echo "first $_\n" >>/tmp/cctv_redi.log`;
                my $path = $1;
                my $port = $2;
                my $redi_ret = "0 http://$path$port\n";
                print $redi_ret;
#`echo "second $redi_ret\n" >>/tmp/cctv_redi.log`;
        }
        else
        { print $_; }
}
