#!/usr/bin/perl
use Getopt::Std;
my %options=();
getopts("o:",\%options);
if(!exists $options{o}){
	        $options{o} = "/data/refresh_db/rf_squid.pid.db";
}

my $squid_conf = "/usr/local/squid/etc/squid.conf";
my $tmp_file = "/tmp/.swap.conf";

my $db_file = $options{o};

system("rm -f $db_file");

`grep "^\s*cache_dir" $squid_conf >$tmp_file`;
if(-z $tmp_file){
	        die "get cache_dir from $squid_conf error!\n";
}

$cmd = "/usr/local/squid/bin/viewswap -f /tmp/.swap.conf |grep \"url =\" |awk '{print \$3}' >$db_file";
`$cmd`;
