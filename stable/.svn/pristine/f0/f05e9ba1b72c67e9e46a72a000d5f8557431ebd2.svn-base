#!/usr/bin/perl


use strict;
$|=1;
while (<>)
{

	if(/key1=+\S+&key2=+\S+&key3=+\S+&key4=+\S+/)
	{
	#	`echo "first $_\n" >>/tmp/6room_redi.log`;
		if(/(\S*) \w+:\/\/(.*)/i){
			my $first = $1;
			my $uri = $2;
			my $redi_ret =  "$first http://cpis.6room.ccgslb.net/$uri";
			print  $redi_ret . "\n";
	#		`echo "second $redi_ret\n" >>/tmp/6room_redi.log`;
		}
	}
	elsif(/\.56.com/)
	{
	#	`echo "first $_\n" >>/tmp/56_redi.log`;
		if(/(\S*) \w+:\/\/(.*)/i){
			my $first = $1;
			my $uri = $2;
			my $redi_ret =  "$first http://cpis.56.ccgslb.net/$uri";
			print  $redi_ret . "\n";
	#		`echo "second $redi_ret\n" >>/tmp/56_redi.log`;
		}
	}
	elsif(/Zmxhc2g-/)
	{
	#	`echo "first $_\n" >>/tmp/cctv_redi.log`;
		if(/(\S*) \w+:\/\/(.*)/i){
			my $first = $1;
			my $uri = $2;
			my $redi_ret =  "$first http://cpis.cctv.ccgslb.net/$uri\n";
			print $redi_ret . "\n";
	#		`echo "second $redi_ret\n" >>/tmp/cctv_redi.log`;
		}
	}
	else { print $_;}
}
