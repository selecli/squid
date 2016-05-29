#!/usr/bin/perl
$|=1;
while (<>)
{
	if(/(\S*) (\w+):\/\/(\S*) (.*)/){
		my $first = $1;
		my $prot  = $2;
		my $uri = $3;
		my $urlgroup = $4;

		if($prot eq "http" || $prot eq "HTTP"){
			my $redi_ret =  "$first http://cpis.longsou.ccgslb.net/$uri $urlgroup\n";
			print $redi_ret;
		}else{
			print $_;
		}   
	}       
	else
	{
		print $_;
	}   
}  
