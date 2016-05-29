#!/usr/bin/perl

my @urls = `grep "mod_customized_server_side_error_page" /usr/local/squid/etc/squid.conf -ri`;
foreach my $s_u(@urls){
	my @val = split(/\s+/,$s_u);
	my $url = $val[1];
	if($url =~ /http:\/\/(.*)/){
		print $1 . "\n";
		my $uri = $1;
		$uri =~ s/\//_/g;

		`touch /data/proclog/log/squid/customized_error_page/$uri; chmod 777 /data/proclog/log/squid/customized_error_page/$uri`;
		system(" wget --timeout=20 -O /data/proclog/log/squid/customized_error_page/$uri $url");
		print $uri . "\n";
	}
}
