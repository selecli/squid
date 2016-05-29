#!/usr/bin/perl

#************************************
#Desc:   check disk write ,read  speed
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
      print "diskSpeedReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"diskSpeedReader -v\" for version number.", "\r\n";
      print "Use \"diskSpeedReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"diskSpeedReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/diskSpeedMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;


$diskWriteSpeed ="-1\r\n";
$diskReadSpeed = "-1\r\n";
$checkUpdateTime = 0;

if(!(-e $poolFile))
{
   print $diskWriteSpeed;
   print $diskReadSpeed;
   print ctime($checkUpdateTime) ;
   exit ;
}

@poolData = `cat /monitor/pool/diskSpeedMon.pool` ;

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
   if(@cs[0]=~/^diskWriteSpeedMBps/)
   {
       $diskWriteSpeed = @cs[1];
   }
   elsif(@cs[0]=~/^diskReadSpeedInMBps/)
       {  
         $diskReadSpeed= @cs[1];
        } 
         elsif(@cs[0]=~/^diskSpeedUpdateTime/)
           {
               $checkUpdateTime = @cs[1]; 
            }
}



$lt = time();

if(($lt - $checkUpdateTime)>=$timeOut)
{
   $diskWriteSpeed= sprintf("-1\r\n") ;
   $diskReadSpeed = sprintf("-1\r\n") ;
}

 print $diskWriteSpeed;
 print $diskReadSpeed;
 print ctime($checkUpdateTime) ;

