#!/usr/bin/perl

#********************************************
#Desc:   check squid  origin response speed
#Author: Qingrong.Jiang
#Date :  2006-03-15
#*******************************************


use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "squidOriginResponseReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidOriginResponseReader v\" for version number.", "\r\n";
      print "Use \"squidOriginResponseReader h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidOriginResponseReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/squidOriginResponseMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;

$checkUpdateTime = -1;
$checkResponseTime = "-1\r\n";
$checkResponseSpeedInKB = "-1\r\n";

if(!(-e $poolFile))
{
   print $checkResponseTime ;
   print $checkResponseSpeedInKB ;
   print ctime($checkUpdateTime) ;
   exit ;
}

@poolData = `cat /monitor/pool/squidOriginResponseMon.pool` ;

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

   if(@cs[0]=~/^squidOriginResponseTime/)
   {
       $checkResponseTime = @cs[1];
   }
      elsif(@cs[0]=~/^squidOriginResponseSpeedInKBps/)
      {
         $checkResponseSpeedInKB = @cs[1];
      }
         elsif(@cs[0]=~/^squidOriginResponseUpdateTime/)
         {
           $checkUpdateTime = @cs[1] ;
         }
}



$lt = time();

if(($lt - $checkUpdateTime)>=$timeOut)
{
   $checkResponseTime = sprintf("-1\r\n") ;
   $checkResponseSpeedInKB = sprintf("-1\r\n") ;
}

 print $checkResponseTime ;
 print $checkResponseSpeedInKB;
 print ctime($checkUpdateTime) ;

