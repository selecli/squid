#!/usr/bin/perl
my $file = "/data/refresh_db/rf_squid.pid.db";
my $file_tmp = "/data/refresh_db/rf_squid.pid.db.tmp";
my $hash;

open FH,"<$file";
while(<FH>){
	$hash->{$_} = 1;
}

close(FH);

open FH,">$file_tmp";
foreach my $key(keys %$hash){
	printf FH "$key";
}

system("mv $file_tmp $file");
system("refreshd -k shutdown");
