#!/usr/bin/perl

#************************************
#Desc:   Read system connection count
#Author: Qingrong.Jiang
#Date :  2006-03-06
#************************************


use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "netConnReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"netConnReader -v\" for version number.", "\r\n";
      print "Use \"netConnReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"netConReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/netConnMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;

$checkUpdateTime = -1;
$checkConn = "-1\r\n";

if(!(-e $poolFile))
{
   print $checkConn ;
   print ctime($checkUpdateTime) ;
   exit ;
}

@poolData = `cat /monitor/pool/netConnMon.pool` ;

if(!(-d $configureFile))
{
  @configure = `cat /monitor/etc/monitor.conf` ;


  #read configure file
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
   if(@cs[0]=~/^netConnCount/)      
   {
       $checkConn = @cs[1];
   }
         elsif(@cs[0]=~/^netConnUpdateTime/)
           {
               $checkUpdateTime = @cs[1]; 
            }
}



$lt = time();

if(($lt - $checkUpdateTime)>=$timeOut)
{
   $checkConn = sprintf("-1\r\n") ;
}


 print $checkConn ;
 print ctime($checkUpdateTime) ;

