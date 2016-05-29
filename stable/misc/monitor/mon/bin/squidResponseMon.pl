#!/usr/bin/perl

#********************************
#Desc:   check squid response speed
#Author: Qingrong.Jiang/zhaobin
#Date :  2012-11-15
#********************************


#**************************************************************
#                Update  Log
#--------------------------------------------------------------
# version 1.4 : 2006-04-27
#       change response time unit : second ---> millisecond ; 
#       fixed by Qingrong.Jiang 
#
#**************************************************************

use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);
use Time::HiRes qw( usleep ualarm gettimeofday tv_interval );
use URI;
use Net::HTTP;
use Socket;
use Data::Dumper;

require 'sys/ioctl.ph';
use Socket;

$version = 1.4;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "squidResponseMon version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidResponseMon -v\" for version number.", "\r\n";
      print "Use \"squidResponseMon -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidResponseMon -h\" for help.", "\r\n";
      exit;
   }
}

@configure = `cat ../etc/monitor.conf` ;

$count = 0;

$chkCount = 3 ;

$n =0;

#read configure file
foreach $Content (@configure)
{
    #$_= $Content ;
     if($Content=~/^#/)
      {
       #  print $Content ;
      }
     elsif($Content=~/^squid_url/)
       {
         @cf = split(/ +/,$Content);
         $Url = @cf[1] ; 
         last;
        }
}


sub selectIP {
    # get all interface info
     my $interface;
my %IPs;

foreach ( qx{ (LC_ALL=C /sbin/ifconfig -a 2>&1) } ) {
        $interface = $1 if /^(\S+?):?\s/;
        next unless defined $interface;
        $IPs{$interface}->{STATE}=uc($1) if /\b(up|down)\b/i;
        $IPs{$interface}->{IP}=$1 if /inet\D+(\d+\.\d+\.\d+\.\d+)/i;
}
    foreach my $inft ( sort keys %IPs ){

                next if $IPs{$inft}->{STATE} eq 'DOWN' || !exists $IPs{$inft}->{IP};
		$ip = $IPs{$inft}->{IP};
#if( $ip !~ /^(127|192|10|172)\./){
		if( $ip !~ /^(127)\./){
                        #print $inft, $ip, "\n";
			return $ip;
		}
	}



#    my %interfaces;
#    my $max_addrs = 30;
#    socket(my $socket, AF_INET, SOCK_DGRAM, 0) or die "socket: $!";
#    
#    {
#        my $ifreqpack = 'a16a16';
#        my $buf = pack($ifreqpack, '', '') x $max_addrs;
#        my $ifconf = pack('iP', length($buf), $buf);
#    
#        # This does the actual work
#        ioctl($socket, SIOCGIFCONF(), $ifconf) or die "ioctl: $!";
#    
#        my $len = unpack('iP', $ifconf);
#        substr($buf, $len) = '';
#    
#        %interfaces = unpack("($ifreqpack)*", $buf);
#    
#        unless (keys(%interfaces) < $max_addrs) {
#            # Buffer was too small
#            $max_addrs += 10;
#            redo;
#        }
#    }
#    
#    for my $addr (values %interfaces) {
#        $addr = inet_ntoa((sockaddr_in($addr))[1]);
#    }
#    # filter
#    my $ip;
#    foreach my $inft ( sort keys %interfaces ){
#		$ip = $interfaces{$inft};
##if( $ip !~ /^(127|192|10|172)\./){
#		if( $ip !~ /^(127)\./){
#			return $ip;
#		}
#	}
return;
}

$u1 = URI->new($Url);
$path = $u1->path();

my $ip = "127.0.0.1";
$headerHost = sprintf("Host: %s", $u1->host());
%header=("Accept","*/*","Host",$u1->host());
#print $ip ;
$s = Net::HTTP->new($ip) or die "Can't connect: $@\n";
$s->write_request("GET", $path , %header);
($code, $mess, %headers) = $s->read_response_headers();
if ($code != "200")
{
	$ip = selectIP();
	die "Can't get public ip" unless $ip;
}
#print "client IP = " ,$ip ,"\n";

$chkNo = 0 ;
while($chkNo < $chkCount )
{
	$chkNo ++ ;

	$s = Net::HTTP->new($ip) or die "Can't connect: $@\n";
	#send resquest
	$s->write_request("GET", $path , %header);
	($sec1 , $micro1) = gettimeofday () ;

	#recv respose header
	($code, $mess, %headers) = $s->read_response_headers();
	$count = $s->_rbuf_length ;

	#recv response body 
	while (1) {
		$buf;
		$n = $s->read_entity_body($buf, 1024);
		if($n<=0)
		{
			die "read failed: $!" unless defined $n;
			last unless $n;
		}
		$count = $count + $n;
   	}

	($sec2 , $micro2) = gettimeofday () ;
	$diff = ($sec2 - $sec1) *1000000 + ($micro2 - $micro1) ;

	#caculate net speed 
	if($diff==0 && $count>0)
	{
		$speedInKB = 1024*10;
	}
	elsif($diff>0 && $count>0)
	{
		$speedInKB = $count*1000000/$diff/1024;
	}
	else
	{
		$speedInKB = -1 ;
	}

	$totalDiff = $totalDiff + $diff ;
	$totalSpeedInKB = $totalSpeedInKB + $speedInKB ;

	sleep(1);
}

$strDiff = sprintf("%d" , $totalDiff/1000/$chkCount);
$strSpeedInKB = sprintf( "%.2f", $totalSpeedInKB/$chkCount);


print "time: ",  $strDiff ," milliseconds \r\n";
print  "speed in real in KB: ", $strSpeedInKB  , " KB/s \r\n";

$lt = time();

$strCmd1 = sprintf("echo squidResponseTime: %s > ../pool/squidResponseMon.pool", $strDiff);
$strCmd2 = sprintf("echo squidResponseSpeedInKBps: %s >> ../pool/squidResponseMon.pool", $strSpeedInKB);
$strCmd3 = sprintf("echo squidResponseUpdateTime: %s >> ../pool/squidResponseMon.pool", $lt);

system($strCmd1);
system($strCmd2);
system($strCmd3);


