#!/usr/bin/perl

#********************************
#Desc:  Read squid Mgr information 
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
      print "squidMgrMonReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidMgrMonReader -v\" for version number.", "\r\n";
      print "Use \"squidMgrMonReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidMgrMonReader -h\" for help.", "\r\n";
      exit;
   }
}

$poolFile = "/monitor/pool/squidMgrMon.pool";
$configureFile = "/monitor/etc/monitor.conf";

$count=0;

$timeOut=300;

$squidCacheSysVMsize = "-1\r\n";
$squidCacheSysStorage = "-1\r\n";
$squidCacheMemUsage = "-1\r\n";
$squidCacheHttpErrors = "-1\r\n";
$squidCacheHttpErrorsIncreased = "-1\r\n";
$squidCacheHttpMissSvcTime1 = "-1\r\n";
$squidCacheHttpMissSvcTime5 = "-1\r\n";
$squidCacheHttpMissSvcTime60 = "-1\r\n";
$squidCacheHttpMissSvcTime1To60 = "-1\r\n";
$squidCacheHttpNmSvcTime1 = "-1\r\n";
$squidCacheHttpNmSvcTime5 = "-1\r\n";
$squidCacheHttpNmSvcTime60 = "-1\r\n";
$squidCacheHttpNmSvcTime1To60 = "-1\r\n";
$squidCacheHttpHitSvcTime1 = "-1\r\n";
$squidCacheHttpHitSvcTime5 = "-1\r\n";
$squidCacheHttpHitSvcTime60 = "-1\r\n";
$squidCacheHttpHitSvcTime1To60 = "-1\r\n";
$squidCacheDnsSvcTime1 = "-1\r\n";
$squidCacheDnsSvcTime5 = "-1\r\n";
$squidCacheDnsSvcTime60 = "-1\r\n";
$squidCacheDnsSvcTime1To60 = "-1\r\n";
$squidCacheRequestHitRatio1 = "-1\r\n";
$squidCacheRequestHitRatio5 = "-1\r\n";
$squidCacheRequestHitRatio60 = "-1\r\n";
$squidCacheRequestHitRatio1To60 = "-1\r\n";
$squidMemHitRatio5 = "-1\r\n";
$squidMemHitRatio60 = "-1\r\n";
$squidMemHitRatio5To60 = "-1\r\n";
$squidDiskHitRatio5 = "-1\r\n";
$squidDiskHitRatio60 = "-1\r\n";
$squidDiskHitRatio5To60 = "-1\r\n";
$squidCacheRequestByteRatio1 = "-1\r\n";
$squidCacheRequestByteRatio5 = "-1\r\n";
$squidCacheRequestByteRatio60 = "-1\r\n";
$squidCacheRequestByteRatio1To60 = "-1\r\n";
$squidMgrChkUpdateTime = -1;

if(!(-e $poolFile))
{
   print $squidCacheSysVMsize;
   print $squidCacheSysStorage;
   print $squidCacheMemUsage;
   print $squidCacheHttpErrors;
   print $squidCacheHttpErrorsIncreased;
   print $squidCacheHttpMissSvcTime1;
   print $squidCacheHttpMissSvcTime5;
   print $squidCacheHttpMissSvcTime60;
   print $squidCacheHttpMissSvcTime1To60;
   print $squidCacheHttpNmSvcTime1;
   print $squidCacheHttpNmSvcTime5;
   print $squidCacheHttpNmSvcTime60;
   print $squidCacheHttpNmSvcTime1To60;
   print $squidCacheHttpHitSvcTime1;
   print $squidCacheHttpHitSvcTime5;
   print $squidCacheHttpHitSvcTime60;
   print $squidCacheHttpHitSvcTime1To60;
   print $squidCacheDnsSvcTime1;
   print $squidCacheDnsSvcTime5;
   print $squidCacheDnsSvcTime60;
   print $squidCacheDnsSvcTime1To60;
   print $squidCacheRequestHitRatio1;
   print $squidCacheRequestHitRatio5;
   print $squidCacheRequestHitRatio60;
   print $squidCacheRequestHitRatio1To60;
   print $squidMemHitRatio5;
   print $squidMemHitRatio60;
   print $squidMemHitRatio5To60;
   print $squidDiskHitRatio5;
   print $squidDiskHitRatio60;
   print $squidDiskHitRatio5To60;
   print $squidCacheRequestByteRatio1;
   print $squidCacheRequestByteRatio5;
   print $squidCacheRequestByteRatio60;
   print $squidCacheRequestByteRatio1To60;
   print $squidMgrChkUpdateTime;
   exit ;
}

@poolData = `cat /monitor/pool/squidMgrMon.pool` ;

if(!(-d $configureFile))
{
 @configure = `cat /monitor/etc/monitor.conf` ;

 foreach $Content(@configure)
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
   if(@cs[0]=~/^squidCacheSysVMsize/)
   {
       $squidCacheSysVMsize = @cs[1];
   }
   elsif(@cs[0]=~/^squidCacheSysStorage/)
   {  
      $squidCacheSysStorage= @cs[1];
   } 
   elsif(@cs[0]=~/^squidCacheMemUsage/)
   {
       $squidCacheMemUsage = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpErrorsIncreased/)
   {
       $squidCacheHttpErrorsIncreased = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpErrors/)
   {
       $squidCacheHttpErrors = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpMissSvcTime1To60/)
   {
       $squidCacheHttpMissSvcTime1To60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpMissSvcTime5/)
   {
       $squidCacheHttpMissSvcTime5 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpMissSvcTime60/)
   {
       $squidCacheHttpMissSvcTime60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpMissSvcTime1/)
   {
       $squidCacheHttpMissSvcTime1 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpNmSvcTime1To60/)
   {
       $squidCacheHttpNmSvcTime1To60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpNmSvcTime5/)
   {
       $squidCacheHttpNmSvcTime5 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpNmSvcTime60/)
   {
       $squidCacheHttpNmSvcTime60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpNmSvcTime1/)
   {
       $squidCacheHttpNmSvcTime1 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpHitSvcTime1To60/)
   {
       $squidCacheHttpHitSvcTime1To60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpHitSvcTime5/)
   {
       $squidCacheHttpHitSvcTime5 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpHitSvcTime60/)
   {
       $squidCacheHttpHitSvcTime60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheHttpHitSvcTime1/)
   {
       $squidCacheHttpHitSvcTime1 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheDnsSvcTime1To60/)
   {
       $squidCacheDnsSvcTime1To60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheDnsSvcTime5/)
   {
       $squidCacheDnsSvcTime5 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheDnsSvcTime60/)
   {
       $squidCacheDnsSvcTime60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheDnsSvcTime1/)
   {
       $squidCacheDnsSvcTime1 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheRequestHitRatio1To60/)
   {
       $squidCacheRequestHitRatio1To60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheRequestHitRatio5/)
   {
       $squidCacheRequestHitRatio5 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheRequestHitRatio60/)
   {
       $squidCacheRequestHitRatio60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheRequestHitRatio1/)
   {
       $squidCacheRequestHitRatio1 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidMemHitRatio5To60/)
   {
       $squidMemHitRatio5To60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidMemHitRatio60/)
   {
       $squidMemHitRatio60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidMemHitRatio5/)
   {
       $squidMemHitRatio5 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidDiskHitRatio5To60/)
   {
       $squidDiskHitRatio5To60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidDiskHitRatio60/)
   {
       $squidDiskHitRatio60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidDiskHitRatio5/)
   {
       $squidDiskHitRatio5 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheRequestByteRatio1To60/)
   {
       $squidCacheRequestByteRatio1To60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheRequestByteRatio5/)
   {
       $squidCacheRequestByteRatio5 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheRequestByteRatio60/)
   {
       $squidCacheRequestByteRatio60 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidCacheRequestByteRatio1/)
   {
       $squidCacheRequestByteRatio1 = @cs[1]; 
   }
   elsif(@cs[0]=~/^squidMgrChkUpdateTime/)
   {
       $squidMgrChkUpdateTime = @cs[1]; 
   }
          
}

$lt = time();

 if(($lt - $squidMgrChkUpdateTime)>=$timeOut)
{
    $squidCacheSysVMsize = sprintf("-1\r\n");
    $squidCacheSysStorage = sprintf("-1\r\n");
    $squidCacheMemUsage = sprintf("-1\r\n");
    $squidCacheHttpErrors = sprintf("-1\r\n");
    $squidCacheHttpErrorsIncreased = sprintf("-1\r\n");
    $squidCacheHttpMissSvcTime1 = sprintf("-1\r\n");
    $squidCacheHttpMissSvcTime5 = sprintf("-1\r\n");
    $squidCacheHttpMissSvcTime60 = sprintf("-1\r\n");
    $squidCacheHttpMissSvcTime1To60 = sprintf("-1\r\n");
    $squidCacheHttpNmSvcTime1 = sprintf("-1\r\n");
    $squidCacheHttpNmSvcTime5 = sprintf("-1\r\n");
    $squidCacheHttpNmSvcTime60 = sprintf("-1\r\n");
    $squidCacheHttpNmSvcTime1To60 = sprintf("-1\r\n");
    $squidCacheHttpHitSvcTime1 = sprintf("-1\r\n");
    $squidCacheHttpHitSvcTime5 = sprintf("-1\r\n");
    $squidCacheHttpHitSvcTime60 = sprintf("-1\r\n");
    $squidCacheHttpHitSvcTime1To60 = sprintf("-1\r\n");
    $squidCacheDnsSvcTime1 = sprintf("-1\r\n");
    $squidCacheDnsSvcTime5 = sprintf("-1\r\n");
    $squidCacheDnsSvcTime60 = sprintf("-1\r\n");
    $squidCacheDnsSvcTime1To60 = sprintf("-1\r\n");
    $squidCacheRequestHitRatio1 = sprintf("-1\r\n");
    $squidCacheRequestHitRatio5 = sprintf("-1\r\n");
    $squidCacheRequestHitRatio60 = sprintf("-1\r\n");
    $squidCacheRequestHitRatio1To60 = sprintf("-1\r\n");
    $squidMemHitRatio5 = sprintf("-1\r\n");
    $squidMemHitRatio60 = sprintf("-1\r\n");
    $squidMemHitRatio5To60 = sprintf("-1\r\n");
    $squidDiskHitRatio5 = sprintf("-1\r\n");
    $squidDiskHitRatio60 = sprintf("-1\r\n");
    $squidDiskHitRatio5To60 = sprintf("-1\r\n");
    $squidCacheRequestByteRatio1 = sprintf("-1\r\n");
    $squidCacheRequestByteRatio5 = sprintf("-1\r\n");
    $squidCacheRequestByteRatio60 = sprintf("-1\r\n");
    $squidCacheRequestByteRatio1To60 = sprintf("-1\r\n");
}

print $squidCacheSysVMsize;
print $squidCacheSysStorage;
print $squidCacheMemUsage;
print $squidCacheHttpErrors;
print $squidCacheHttpErrorsIncreased;
print $squidCacheHttpMissSvcTime1;
print $squidCacheHttpMissSvcTime5;
print $squidCacheHttpMissSvcTime60;
print $squidCacheHttpMissSvcTime1To60;
print $squidCacheHttpNmSvcTime1;
print $squidCacheHttpNmSvcTime5;
print $squidCacheHttpNmSvcTime60;
print $squidCacheHttpNmSvcTime1To60;
print $squidCacheHttpHitSvcTime1;
print $squidCacheHttpHitSvcTime5;
print $squidCacheHttpHitSvcTime60;
print $squidCacheHttpHitSvcTime1To60;
print $squidCacheDnsSvcTime1;
print $squidCacheDnsSvcTime5;
print $squidCacheDnsSvcTime60;
print $squidCacheDnsSvcTime1To60;
print $squidCacheRequestHitRatio1;
print $squidCacheRequestHitRatio5;
print $squidCacheRequestHitRatio60;
print $squidCacheRequestHitRatio1To60;
print $squidMemHitRatio5;
print $squidMemHitRatio60;
print $squidMemHitRatio5To60;
print $squidDiskHitRatio5;
print $squidDiskHitRatio60;
print $squidDiskHitRatio5To60;
print $squidCacheRequestByteRatio1;
print $squidCacheRequestByteRatio5;
print $squidCacheRequestByteRatio60;
print $squidCacheRequestByteRatio1To60;
print ctime($squidMgrChkUpdateTime) ;
