#!/usr/bin/perl

#************************************
#Desc:   check disk space speed
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
      print "diskSpaceReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"diskSpaceReader -v\" for version number.", "\r\n";
      print "Use \"diskSpaceReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"diskSpaceReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/diskSpaceMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;


$diskFreeSpace ="-1\r\n";
$diskUsedPercent = "-1\r\n";
$diskFreeSapceChkSection = "-1\r\n";
$diskUsedPercentChksection = "-1\r\n";
$checkUpdateTime = 0;

if(!(-e $poolFile))
{
   print $diskFreeSpace;
   print $diskUsedPercent ;
   print ctime($checkUpdateTime) ;
   exit ;
}

@poolData = `cat /monitor/pool/diskSpaceMon.pool` ;

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
   if(@cs[0]=~/^diskFreeSpaceMB/)      
   {
       $diskFreeSpace = @cs[1];
   }
   elsif(@cs[0]=~/^diskUsedSpacePercentChkSection/)
       {  
          $diskUsedPercentChksection = @cs[1];
        } 
      elsif (@cs[0]=~/^diskFreeSpaceChkSection/)
         {
            $diskFreeSapceChkSection = @cs[1];
         }
        elsif(@cs[0]=~/^diskUsedSpacePercent/)
          {
            $diskUsedPercent = @cs[1];
          }
         elsif(@cs[0]=~/^diskSpaceUpdateTime/)
           {
               $checkUpdateTime = @cs[1]; 
            }
}



$lt = time();

if(($lt - $checkUpdateTime)>=$timeOut)
{
   $diskFreeSpace = "-1\r\n" ;
   $diskUsedPercent = "-1\r\n" ;
   $diskFreeSapceChkSection = "-1\r\n";
   $diskUsedPercentChksection = "-1\r\n";
}

 print $diskFreeSpace ;
 print $diskUsedPercent;
 print $diskFreeSapceChkSection;
 print $diskUsedPercentChksection ;
 print ctime($checkUpdateTime) ;

