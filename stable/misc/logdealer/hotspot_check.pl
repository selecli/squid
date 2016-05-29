#!/usr/bin/perl
use strict;
use Errno qw(EINTR);
$| = 1;
$SIG{USR1} = \&m_handle_sig;
my %hash;

my $log_flag = 0;
my $log;
open $log,">>/data/hotspot/helper.log";
my $sig_flag = 0;

sub logme{
	print $log shift if($log_flag);
}

sub m_handle_sig{
	$sig_flag = 1;
}
sub read_data{
	%hash = ();
	open(IN,"/data/hotspot/hot_spot.result")||die "Can not open:$!";
	while(<IN>){
		chomp;
		$hash{$_} = 1;
	}
	close(IN);
}

read_data();

while (1)
{
	if(eof(STDIN)){
		logme("stdin closed ...\n");
		goto OUT;
	}
	
	my $ln = <STDIN>;

	if($sig_flag){
		logme("recv sig usr1\n");
		read_data();
		$sig_flag = 0;
	}

	chomp $ln;
	logme("query url : $ln\n");
	unless(length($ln)){
		next;
	}
	if(exists $hash{$ln}){
		logme("--OK\n");
		print "OK\n";
	}
	else{
		logme("--ERR\n");
		print "ERR\n";
	}
}

if($! == EINTR){
	logme("singal interrupt!\n");
}
#goto Again;
OUT:
logme("exit now!\n");
