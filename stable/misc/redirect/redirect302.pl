#!/usr/bin/perl
$|=1;
while (<>)
{
	if(/(\w+):\/\/([^\/]+)\/(\S+)/){
		my $prot = $1;
		my $hostname = $2;

		if($hostname =~ /:(\d+)/){
		}

		my $uri = $3;
		if($prot eq "http" || $prot eq "HTTP"){
			if($uri !~ /www_chinacache_com/){
				print "302:" . "https://$hostname/www_chinacache_com/$uri";
			}
			else{
				$uri =~ s/www_chinacache_com\///g;
				print "http://$hostname/$uri";
			}
		}
		else{
			$uri =~ s/www_chinacache_com\///g;
			print "$prot://$hostname/$uri";
		}
	}
	else{
		print $_;
	}
}
