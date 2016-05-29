#!/usr/bin/perl
$|=1;
while (<>)
{

        if(/(\S*) (\w+):\/\/([^\/]+)\/(\S*) (.*)/){
                my $first = $1;
                my $prot  = $2;
                my $hostname = $3;
                my $uri = $4;
                my $urlgroup = $5;

                if($prot eq "http" || $prot eq "HTTP"){
			if($hostname eq "127.0.0.1:8909"){

		        	my $redi_ret =  "$first http://ccflvcache.ccgslb.net/$uri $urlgroup\n";
                                print $redi_ret;

			}		
                        elsif($hostname !~ /ccflvcache.ccgslb.net/){
                                my $redi_ret =  "$first http://ccflvcache.ccgslb.net/$hostname/$uri $urlgroup\n";

                                print $redi_ret;
                        }
                        else{
                                print $_;
                        }
                }
                else{
                        print $_;
                }
        }
        else{
                print $_;
        }
}
