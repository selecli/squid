#!/usr/bin/perl

#************************************
#Desc:   check squid response speed
#Author: Qingrong.Jiang
#Date :  2006-03-06
#************************************

#************************************************************
#          update  log 
#
#version  1.4  , fix "checkupdatetime" bug . fixed by Jiangqr
#
#************************************************************
 

use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.4;

if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "squidResponseReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidResponseReader -v\" for version number.", "\r\n";
      print "Use \"squidResponseReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidResponseReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/squidResponseMon.pool";
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

@poolData = `cat /monitor/pool/squidResponseMon.pool` ;

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
   
   if(@cs[0]=~/^squidResponseTime/)
   {
       $checkResponseTime = @cs[1];
   }
      elsif(@cs[0]=~/^squidResponseSpeedInKBps/)
      {
         $checkResponseSpeedInKB = @cs[1];
      }
    	 elsif(@cs[0]=~/^squidResponseUpdateTime/)
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

