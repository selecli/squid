#!/usr/bin/perl

#************************************
#Desc:   check net connection count
#Author: Qingrong.Jiang
#Date :  2006-03-16
#************************************


use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "netBandwidthReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"netBandwidthReader -v\" for version number.", "\r\n";
      print "Use \"netBandwidthReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"netBandwidthReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/netBandwidthMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;

$checkUpdateTime = -1;
$checkResponseTime = "-1\r\n";
$checkResponseSpeedInKB = "-1\r\n";

$checkBandwidthIn= sprintf("-1\r\n");
$checkBandwidthOut=sprintf("-1\r\n");

if(!(-e $poolFile))
{
   print $checkResponseTime ;
   print $checkResponseSpeedInKB ;
   print ctime($checkUpdateTime) ;
   exit ;
}

@poolData = `cat /monitor/pool/netBandwidthMon.pool` ;

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
   if(@cs[0]=~/^netBandwidthInKBps/)
   {
       $checkBandwidthIn = @cs[1];
   }
   elsif(@cs[0]=~/^netBandwidthInOutKBps/)
       {  
          $checkBandwidthOut = @cs[1];
        } 
         elsif(@cs[0]=~/^netBandwidthUpdateTime/)
           {
               $checkUpdateTime = @cs[1]; 
            }
}



$lt = time();

if(($lt - $checkUpdateTime)>=$timeOut)
{
   $checkBandwidthIn = sprintf("-1\r\n") ;
   $checkBandwidthOut = sprintf("-1\r\n") ;
}

 print $checkBandwidthIn ;
 print $checkBandwidthOut;
 print ctime($checkUpdateTime) ;

