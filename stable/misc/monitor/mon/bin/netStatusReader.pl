#!/usr/bin/perl

#********************************
#Desc:  Get net Status information 
#Author: LuoJun.Zeng
#Date :  2006-04-15
#********************************

use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "netStatusReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"netStatusReader -v\" for version number.", "\r\n";
      print "Use \"netStatusReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"netStatusReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/netStatusMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;


$netStatusMaxErrosDiff ="-1\r\n";
$netStatusMaxErrosDiffDev = "-1\r\n";
$netStatusMaxCollsDiff = "-1\r\n";
$netStatusMaxCollsDiffDev = "-1\r\n";
$netStatusUpdateTime = -1;

if(!(-e $poolFile))
{
   print $netStatusMaxErrosDiff;
   print $netStatusMaxErrosDiffDev;
   print $netStatusMaxCollsDiff;
   print $netStatusMaxCollsDiffDev;
   print ctime($netStatusUpdateTime) ;
   exit ;
}

@poolData = `cat /monitor/pool/netStatusMon.pool` ;

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
   if(@cs[0]=~/^netStatusMaxErrosDiffDev/)
   {
       $netStatusMaxErrosDiffDev = @cs[1];
   }
   elsif(@cs[0]=~/^netStatusMaxErrosDiff/)
       {  
         $netStatusMaxErrosDiff= @cs[1];
        } 
         elsif(@cs[0]=~/^netStatusMaxCollsDiffDev/)
           {
               $netStatusMaxCollsDiffDev = @cs[1]; 
            }
            elsif(@cs[0]=~/^netStatusMaxCollsDiff/)
              {
                  $netStatusMaxCollsDiff = @cs[1]; 
               }
                elsif(@cs[0]=~/^netStatusUpdateTime/)
                 {
                     $netStatusUpdateTime = @cs[1]; 
                  }
}

$lt = time();

 if(($lt - $netStatusUpdateTime)>=$timeOut)        
 {                                                 
    $netStatusMaxErrosDiff =sprintf("-1\r\n");     
    $netStatusMaxErrosDiffDev = sprintf("-1\r\n"); 
    $netStatusMaxCollsDiff = sprintf("-1\r\n");    
    $netStatusMaxCollsDiffDev = sprintf("-1\r\n"); 
 }                                              

print $netStatusMaxErrosDiff;
print $netStatusMaxErrosDiffDev;
print $netStatusMaxCollsDiff;
print $netStatusMaxCollsDiffDev;
print ctime($netStatusUpdateTime) ;

