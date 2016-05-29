#!/usr/bin/perl
use strict;
use Data::Dumper;
my $hash;

my $fatal_num = 0;
my $err_num = 0;
my $warn_num = 0;

my @fatals;
my @errors;
my @warns;

my $debug_level = 0;
 
read_anyhost();
#print Dumper($hash);
sub logme{
	if($debug_level == 1){
		print STDERR @_;
	}

}
sub calc{
	my $hn = $_[0];
	my $ls = substr $hn, -1;
	my $Cname;
	$hn .= "." if ($ls ne ".");
	
	if(exists $hash->{$hn}){
		my $arr = $hash->{$hn};
		my $flag_ipdown = 0;
		my $flag_ipwork = 0;
		my $flag_ipwarn = 0;
		my $flag_ipbad = 0;

		my $flag_ipwarn_in = 0;
		my $flag_ipbad_in= 0;

		foreach my $ea(@$arr){
			if($ea->[2] eq "CNAME"){
				logme("$hn : CNAME\n");
				return;
			}

			if($ea->[7] ne "ip_down"){
				$flag_ipdown = 1;
			}

			if($ea->[6] eq "ip"){
				$flag_ipwork = 1;
			}

			if($ea->[9] eq "Y" and $ea->[7] eq "ip_down" ){
				$flag_ipwarn = 1;
				
			}

			if($ea->[9] eq "Y" and $ea->[7] eq "ip_bad"){
				$flag_ipbad = 1;
			}
			

		}
		my $inactive_a = ';'.$hn;	
		if(exists $hash->{$inactive_a}){
			my $arr_in = $hash->{$inactive_a};
			foreach my $ea_in(@$arr_in){

				if($ea_in->[9] eq "Y" and $ea_in->[7] eq "ip_bad"){
					$flag_ipbad = 1;
				}
				if($ea_in->[9] eq "Y" and $ea_in->[7] eq "ip_down"){
					$flag_ipwarn = 1;
				}
			}

		}

		if(!$flag_ipdown){
			push @fatals,$hn;
			$fatal_num++;
			logme("fatal : $hn\n");
		}

		if(!$flag_ipwork){
			push @errors,$hn;
			$err_num++;
			logme("error :  $hn \n");
		}

		if($flag_ipwarn and $flag_ipdown and $flag_ipwork) {
#		if($flag_ipwarn){
			push @warns,$hn;
			$warn_num++;
			logme("ipdown :  $hn \n");
		}

		elsif($flag_ipbad and $flag_ipdown and $flag_ipwork) {
#		if($flag_ipbad){
			push @warns,$hn;
			$warn_num++;
			logme("ipbad :  $hn \n");
		}
	}
	else{
		push @errors,$hn;
		$err_num++;
		logme("do not find : $hn \n");
	}


}

sub read_anyhost{
	open afh , "</var/named/chroot/var/named/anyhost" || die "open anyhost error!";
	while(<afh>){
		my @a = split;
		if(exists $hash->{$a[0]}){
			my $arr = $hash->{$a[0]};
			push @$arr,\@a;
		}
		else{
			 $hash->{$a[0]} = [\@a];
		}
	}
	close(afh);
}

open fh , "</usr/local/squid/etc/domain.conf" || die "open domain.conf error!";
while(<fh>){
	next if(/^\s*#/);
	next if(/^\s*$/);
	chomp;
	
	my @a = split (/\s+/,$_);

	#$a[1] -> info
	next if($a[1] !~ "N");# || $a[1] ne "n");

	#$a[0] -> hostname
	my $hn = $a[0];
	calc($a[0]);
#	my $inactive_a = ';'.$hn;	
#	calc($inactive_a);
	
}
close(fh);

my $ret_level = ($fatal_num > 0 ? 1 : 0) * 4000 +
	($err_num > 0  ? 1 : 0) * 2000 +
	($warn_num > 0  ? 1 : 0) * 1000;

my $ret = $ret_level + $fatal_num + $err_num + $warn_num ;
logme("output ---------------------------------------------\n");
logme("ret_level : $ret_level, ret : $ret\n");

print "$ret\n";
print "\nFatal($fatal_num):\n" . join(" ", @fatals);
print "\nError($err_num):\n" . join(" ", @errors);
print "\nWarn($warn_num):\n" . join(" ", @warns);
