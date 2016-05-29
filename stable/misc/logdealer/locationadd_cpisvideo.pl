#!/usr/bin/perl

use strict;
use warnings;
use lib "/usr/local/squid/bin/";
use IPv4Addr qw(:all);

open FH, "< /usr/local/squid/etc/cidrlist.conf";
open CPISLOG, ">> /data/proclog/log/squid/cpiscidr.log";

my ($ip1,$cidr1);
my $line;
my $count = 0;
my @globle;

# read CIDR config file and parse it
while($line=readline FH)
{
	# get a line from config file
	unless ( defined $line )
	{
		next if eof;
		die $! if $!;
	}

	$_ = $line;
	s/\s*$//g;
	s/^\s*//g;
	if ( $_ eq "" ) { next; }

	#check the valid CIDR
	if(/(.+)\/(\d+);/)
	{
		if (!ipv4_chkip($1))
		{
			next;
		}
		if ($2 > 32 || $2 < 0)
		{
			next;
		}
	} 
	else 
	{
		next;
	}

	#get the valid cidr
	$globle[$count] = "$1/$2";
	$count++;
}
close(FH);

# fit return 1 otherwise 0
sub checkIPcidr($)
{
	my $counta = 0;
	my $first = $_[0];
	# check whether ip fit cidr 
	if(ipv4_chkip($first))
	{
		foreach(@globle)
		{
			if (ipv4_in_network( $_,  $first))
			{
				$counta = 1;
				last;
			}
		}
	}else
	{
		$counta = 1;
	}
	return $counta;
}

$|=1;
while (<>)
{
	if(/(\d+) http:\/\/([^\/]+)\/(\S*youku\S*) (.*)/){
		my $first = $1;
		my $hostname = $2;
		my $uri = $3;
		my $urlgroup = $4;
		my $time = time();
		my $length = @globle;
		if($hostname eq "127.0.0.1:8909")
		{
			if($uri =~ /([^\/]+)\/(\S*youku.+)/)
			{
				my $status = checkIPcidr($1);
				#ip doesnot fit cidr add some url
				if($status eq "0")
				{
					if($length != 0)
					{
						syswrite(CPISLOG,"$time youku NoMobileIp $1 http://127.0.0.1:8909/$uri\n");
					}
					$uri = "ccflvcache.yk.ccgslb.net/$1/$2";
					my $redi_ret =  "$first http://127.0.0.1:8909/$uri $urlgroup\n";
					print $redi_ret;
				}
				#ip fit cidr
				else
				{
					if($length != 0)
					{
						syswrite(CPISLOG,"$time youku MobileIp $1 http://127.0.0.1:8909/$uri\n");
					}
					my $redi_ret =  "$first http://127.0.0.1:8909/$uri $urlgroup\n";
					print $redi_ret;
				}
			}
			# the uri is not "/ip/*youku*" mode
			else{
				if($length != 0)
				{
					syswrite(CPISLOG,"$time youku NoMobileIp $1 http://127.0.0.1:8909/$uri\n");
				}
				my $redi_ret =  "$first http://127.0.0.1:8909/$uri $urlgroup\n";
				print $redi_ret;
			}
		}
		elsif($hostname !~ /ccflvcache.yk.ccgslb.net/)
		{
			my $status = checkIPcidr($hostname);
			# ip doesnot fit cidr
			if($status eq "0")
			{
				if($length != 0)
				{
					syswrite(CPISLOG,"$time youku NoMobileIp $hostname http://127.0.0.1:8909/$uri\n");
				}
				my $redi_ret =  "$first http://ccflvcache.yk.ccgslb.net/$hostname/$uri $urlgroup\n";
				print $redi_ret;
			}
			# ip fit cidr
			else
			{
				if($length != 0)
				{
					syswrite(CPISLOG,"$time youku MobileIp $hostname http://127.0.0.1:8909/$uri\n");
				}
				my $redi_ret =  "$first http://$hostname/$uri $urlgroup\n";
				print $redi_ret;
			}
		}
		else{
			if($length != 0)
			{
				syswrite(CPISLOG,"$time youku MobileIp $hostname http://127.0.0.1:8909/$uri\n");
			}
			print $_;
		}
	}
	elsif(/key1=+\S+&key2=+\S+&key3=+\S+&key4=+\S+/)
        {
                if(/(\d+) \w+:\/\/(.*)/i){
                        my $first = $1;
                        my $uri = $2;
                        my $redi_ret =  "$first http://cpis.6room.ccgslb.net/$uri\n";
                        print  $redi_ret ;
                }
        }
	elsif(/\.56\.com/)
        {
                if(/(\d+) \w+:\/\/(.*)/i){
                        my $first = $1;
                        my $uri = $2;
                        my $redi_ret =  "$first http://cpis.56.ccgslb.net/$uri\n";
                        print  $redi_ret ;
                }
        }
        elsif(/Zmxhc2g-/)
        {
                if(/(\d+) \w+:\/\/(.*)/i){
                        my $first = $1;
                        my $uri = $2;
                        my $redi_ret =  "$first http://cpis.cctv.ccgslb.net/$uri\n";
                        print $redi_ret ;
                }
        }
        elsif(/(\d+) http:\/\/([^\/]+\/\S+\.video\.qq\.com\/.*)/)
        {
                my $first = $1;
                my $uri  = $2;

                my $redi_ret =  "$first http://cpis.qqvideo.ccgslb.net/$uri\n";
                print  $redi_ret ;
        }
        elsif(/\?key=.*&v=.*/)
        {
                if(/(\d+) \w+:\/\/(.*)/i){
                        my $first = $1;
                        my $uri = $2;
                        my $redi_ret =  "$first http://cpis.qiyi.ccgslb.net/$uri\n";
                        print $redi_ret ;
                }
        }
        elsif(/&id=ku6_vod/)
        {
                if(/(\d+) \w+:\/\/(.*)/i){
                        my $first = $1;
                        my $uri = $2;
                        my $redi_ret =  "$first http://cpis.ku6.ccgslb.net/$uri\n";
                        print $redi_ret ;
                }
        }
        elsif(/(\d+) http:\/\/([^\/]+:8080\/.*\.mp4\?key=.*)/i)
        {
                my $first = $1;
                my $uri = $2;
                my $redi_ret =  "$first http://cpis.pplive.ccgslb.net/$uri\n";
                print $redi_ret ;
        }
	else { print $_;}
}
close(CPISLOG);

