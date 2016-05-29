#!/usr/bin/perl
$|=1;
while (<>)
{


	if (/(\d+) http:\/\/(.*)/) {
		my $first = $1;
		my $uri = $2;

		my $redi_ret =  "$first http://top.sohu.ccgslb.net/$uri\n";
		print $redi_ret;

	}
	else{
		print $_;
	}
}
