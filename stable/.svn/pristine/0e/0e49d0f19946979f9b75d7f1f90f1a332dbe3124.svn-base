#!/usr/bin/perl

#************************************
#Desc:   load check reader .
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
      print "loadReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"loadReader -v\" for version number.", "\r\n";
      print "Use \"loadReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"loadReader -h\" for help.", "\r\n";
      exit;
   }
}

@poolData = `cat /monitor/pool/loadMon.pool` ;
@configure = `cat /monitor/etc/monitor.conf` ;

$count=0;

$timeOut=300;

$checkUpdateTime = 0;
$checkResponseTime = 0;
$checkResponseSpeedInKB = 0;

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

#read check data
foreach $Content(@poolData)
{
   @cs = split(/ +/, $Content);
   if(@cs[0]=~/^loadAve15/)
   {
       $checkLoadAve15 = @cs[1];
   }
   elsif(@cs[0]=~/^loadAve5/)
       {
            $checkLoadAve5 = @cs[1];
        }
      elsif(@cs[0]=~/^loadAve1/)
           {
              $checkLoadAve1 = @cs[1];
           }
   	       elsif(@cs[0]=~/^loadUpdateTime/)
        	   {
                	 $checkUpdateTime = @cs[1];
           	   }
}



$lt = time();

if(($lt - $checkUpdateTime)>=$timeOut)
{
   $checkLoadAve1 = sprintf("-1\r\n") ;
   $checkLoadAve5 = sprintf("-1\r\n") ;
   $checkLoadAve15 = sprintf("-1\r\n") ;
}

 print $checkLoadAve1;
 print $checkLoadAve5;
 print $checkLoadAve15;
 print ctime($checkUpdateTime) ;
