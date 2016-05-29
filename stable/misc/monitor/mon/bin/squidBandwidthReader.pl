#!/usr/bin/perl

#************************************
#Desc:   check squid bandwidth 
#Author: Qingrong.Jiang
#Date :  2006-03-15
#************************************


use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "squidBandwidthReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidBandwidthReader -v\" for version number.", "\r\n";
      print "Use \"squidBandwidthReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidBandwidthReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/squidBandwidthMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;

$checkUpdateTime = -1;
$checkResponseTime = "-1\r\n";
$checkResponseSpeedInKB = "\r\n";

$clientInBandwidth = "-1\r\n";
$clientOutBandwidth = "-1\r\n";
$serverInBandwidth = "-1\r\n";
$serverOutBandwidth = "-1\r\n";

if(!(-e $poolFile))
{
   print $checkResponseTime ;
   print $checkResponseSpeedInKB ;
   print ctime($checkUpdateTime) ;
   exit ;
}

@poolData = `cat /monitor/pool/squidBandwidthMon.pool` ;

if(!(-d $configureFile))
{

 @configure = `cat /monitor/etc/monitor.conf` ;

 foreach $Content (@configure)
  { 
    #$_= $Content ; 
     if($Content=~/^#/)
      {
       #  print $Content ;
      }
     elsif($Content=~/^timeout/)
       {
         $pos = index($Content,"timeout")+ length("timeout ") ;
         $len = length($Content) - $pos -1;
         $timeOut = substr($Content , $pos,$len);
        }
  }
}

#read check data 
foreach $Content(@poolData)
{
   @cs = split(/ +/, $Content);
   if(@cs[0]=~/^squidClientInBandwidthInKBps/)      
   {
       $clientInBandwidth = @cs[1];
   }
   elsif(@cs[0]=~/^squidClinetOutBandwidthInKBps/)
       {  
          $clientOutBandwidth = @cs[1];
        } 
         elsif(@cs[0]=~/^squidServerInBandwidthinKBps/)
           {
               $serverInBandwidth = @cs[1]; 
            }
         elsif (@cs[0]=~/^squidServerOutBandwidthInKBps/)
           {
              $serverOutBandwidth = @cs[1];
           }
            else
            {
               $checkUpdateTime = @cs[1];
            }
}



$lt = time();

if(($lt - $checkUpdateTime)>=$timeOut)
{
	$clientInBandwidth = "-1\r\n";
	$clientOutBandwidth = "-1\r\n";
	$serverInBandwidth = "-1\r\n";
	$serverOutBandwidth = "-1\r\n";
}

print $clientInBandwidth;
print $clientOutBandwidth;
print $serverInBandwidth ;
print $serverOutBandwidth ; 

print ctime($checkUpdateTime) ;

