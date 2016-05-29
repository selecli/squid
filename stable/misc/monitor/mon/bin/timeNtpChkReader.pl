#!/usr/bin/perl

#********************************
#Desc:  Read NtpDate response information 
#Author: LuoJun.Zeng
#Date :  2006-04-15
#********************************

use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV ==1 and !($ARGV[0] cmp "-v"))
   {
      print "timeNtpChkReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"timeNtpChkReader -v\" for version number.", "\r\n";
      print "Use \"timeNtpChkReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"timeNtpChkReader -h\" for help.", "\r\n";
      exit;
   }
}
$poolFile = "/monitor/pool/timeNtpChkMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;


$timeNtpChkResult ="-1\r\n";
$timeNtpChkOffset = "-1\r\n";
$timeNtpChkUpdateTime = -1;

if(!(-e $poolFile))
{
   print $timeNtpChkResult;
   print $timeNtpChkOffset;
   print ctime($timeNtpChkUpdateTime) ;
   exit ;
}

@poolData = `cat /monitor/pool/timeNtpChkMon.pool` ;

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
   if(@cs[0]=~/^timeNtpChkResult/)
   {
       $timeNtpChkResult = @cs[1];
   }
   elsif(@cs[0]=~/^timeNtpChkOffset/)
       {  
         $timeNtpChkOffset= @cs[1];
        } 
         elsif(@cs[0]=~/^timeNtpChkUpdateTime/)
           {
               $timeNtpChkUpdateTime = @cs[1]; 
            }
          
}

$lt = time();

 if(($lt - $timeNtpChkUpdateTime)>=$timeOut)
{
   $timeNtpChkResult =sprintf("-1\r\n");
   $timeNtpChkOffset = sprintf("-1\r\n");
}

print $timeNtpChkResult;
print $timeNtpChkOffset;
print ctime($timeNtpChkUpdateTime) ;
