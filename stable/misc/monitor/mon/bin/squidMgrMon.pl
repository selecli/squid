#!/usr/bin/perl

#********************************
#Desc:  Get squid Mgr information 
#Author: LuoJun.Zeng
#Date :  2006-04-15
#********************************

#**************************************************************
#                Update  Log
#--------------------------------------------------------------
# version 1.4 : 2006-04-27
#       add: save temp device data to /tmp/netDev2.dat
#       fixed by LuoJun.Zeng
#**************************************************************

use Net::SNMP;
use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);
use Time::HiRes qw( usleep ualarm gettimeofday tv_interval );
use URI;
use Net::HTTP;



my $hostname='127.0.0.1';
my $port='3401';
my $community='public';

$count = 0;
$path = "cache_object://localhost/info";
$chkNo = 0 ;

$version = 1.4;
if(@ARGV > 0)
{
   if(@ARGV ==1 and !($ARGV[0] cmp "-v"))
   {
      print "squidMgrMon Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidMgrMon -v\" for version number.", "\r\n";
      print "Use \"squidMgrMon -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidMgrMon -h\" for help.", "\r\n";
      exit;
   }
}

$SNMP::debugging = 0;
$SNMP::dump_packet = 0;
my ($sess, $error) = Net::SNMP->session(
   -hostname  => shift || '127.0.0.1',
   -community => shift || 'public',
   -port      => shift || 3401
);

$squidCacheSysVMsize = "-1";
$squidCacheSysStorage = "-1";
$squidCacheMemUsage = "-1";
$squidCacheHttpErrors = "-1";
$squidCacheHttpErrorsIncreased = "-1";
$squidCacheHttpMissSvcTime1 = "-1";
$squidCacheHttpMissSvcTime5 = "-1";
$squidCacheHttpMissSvcTime60 = "-1";
$squidCacheHttpMissSvcTime1To60 = "-1";
$squidCacheHttpNmSvcTime1 = "-1";
$squidCacheHttpNmSvcTime5 = "-1";
$squidCacheHttpNmSvcTime60 = "-1";
$squidCacheHttpNmSvcTime1To60 = "-1";
$squidCacheHttpHitSvcTime1 = "-1";
$squidCacheHttpHitSvcTime5 = "-1";
$squidCacheHttpHitSvcTime60 = "-1";
$squidCacheHttpHitSvcTime1To60 = "-1";
$squidCacheDnsSvcTime1 = "-1";
$squidCacheDnsSvcTime5 = "-1";
$squidCacheDnsSvcTime60 = "-1";
$squidCacheDnsSvcTime1To60 = "-1";
$squidCacheRequestHitRatio1 = "-1";
$squidCacheRequestHitRatio5 = "-1";
$squidCacheRequestHitRatio60 = "-1";
$squidCacheRequestHitRatio1To60 = "-1";
$squidMemHitRatio5 = "-1";
$squidMemHitRatio60 = "-1";
$squidMemHitRatio5To60 = "-1";
$squidDiskHitRatio5 = "-1";
$squidDiskHitRatio60 = "-1";
$squidDiskHitRatio5To60 = "-1";
$squidCacheRequestByteRatio1 = "-1";
$squidCacheRequestByteRatio5 = "-1";
$squidCacheRequestByteRatio60 = "-1";
$squidCacheRequestByteRatio1To60 = "-1";
$squidMgrChkUpdateTime = -1;

#a)
$squidCacheSysVMsizeOID= '.1.3.6.1.4.1.3495.1.1.1' ;
$squidCacheSysVMsize_res = $sess->get_request(-varbindlist =>[$squidCacheSysVMsizeOID]);
$squidCacheSysVMsize = $squidCacheSysVMsize_res->{$squidCacheSysVMsizeOID} ;

#b)
$squidCacheSysStorageOID = '.1.3.6.1.4.1.3495.1.1.2' ;
$squidCacheSysStorage_res = $sess->get_request(-varbindlist =>[$squidCacheSysStorageOID]);
$squidCacheSysStorage = $squidCacheSysStorage_res->{$squidCacheSysStorageOID} ;

#c)
$squidCacheMemUsageOID = '.1.3.6.1.4.1.3495.1.3.1.3' ;
$squidCacheMemUsage_res = $sess->get_request(-varbindlist =>[$squidCacheMemUsageOID]);
$squidCacheMemUsage = $squidCacheMemUsage_res->{$squidCacheMemUsageOID} ;

#d)
$squidCacheHttpErrorsOID = '.1.3.6.1.4.1.3495.1.3.2.1.3' ;
$squidCacheHttpErrors_res = $sess->get_request(-varbindlist =>[$squidCacheHttpErrorsOID]);
$squidCacheHttpErrors = $squidCacheHttpErrors_res->{$squidCacheHttpErrorsOID};

#$squidCacheHttpErrorsIncreased
$Errors = sprintf("%s", $squidCacheHttpErrors);
$cmdSaveErrors = sprintf("echo %s > /tmp/sq_CacheHttpErrors.tmp", $Errors) ;

#get stale data from tmp file , /tmp/sq_bandwidth.tmp
@fileData=`cat /tmp/sq_CacheHttpErrors.tmp` ;
$line =  @fileData[0] ;
  
@data= split(/ +/, $line);
$s_squidCacheHttpErrors = @data[0];
if($squidCacheHttpErrors - $s_squidCacheHttpErrors >= 0)
{
   $squidCacheHttpErrorsIncreased = $squidCacheHttpErrors - $s_squidCacheHttpErrors;
}
else
{
    $squidCacheHttpErrorsIncreased = 0;
}

#e)
$squidCacheHttpMissSvcTime1OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.3.1' ;
$squidCacheHttpMissSvcTime1_res = $sess->get_request(-varbindlist =>[$squidCacheHttpMissSvcTime1OID]);
$squidCacheHttpMissSvcTime1 = $squidCacheHttpMissSvcTime1_res->{$squidCacheHttpMissSvcTime1OID};

$squidCacheHttpMissSvcTime5OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.3.5' ;
$squidCacheHttpMissSvcTime5_res = $sess->get_request(-varbindlist =>[$squidCacheHttpMissSvcTime5OID]);
$squidCacheHttpMissSvcTime5 = $squidCacheHttpMissSvcTime5_res->{$squidCacheHttpMissSvcTime5OID};

$squidCacheHttpMissSvcTime60OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.3.60' ;
$squidCacheHttpMissSvcTime60_res = $sess->get_request(-varbindlist =>[$squidCacheHttpMissSvcTime60OID]);
$squidCacheHttpMissSvcTime60 = $squidCacheHttpMissSvcTime60_res->{$squidCacheHttpMissSvcTime60OID};

if($squidCacheHttpMissSvcTime60 == 0)
{
   $squidCacheHttpMissSvcTime1To60 = 0;
}
else
{
   $squidCacheHttpMissSvcTime1To60 = $squidCacheHttpMissSvcTime1 / $squidCacheHttpMissSvcTime60;
}

#f)
$squidCacheHttpNmSvcTime1OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.4.1' ;
$squidCacheHttpNmSvcTime1_res = $sess->get_request(-varbindlist =>[$squidCacheHttpNmSvcTime1OID]);
$squidCacheHttpNmSvcTime1 = $squidCacheHttpNmSvcTime1_res->{$squidCacheHttpNmSvcTime1OID};

$squidCacheHttpNmSvcTime5OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.4.5' ;
$squidCacheHttpNmSvcTime5_res = $sess->get_request(-varbindlist =>[$squidCacheHttpNmSvcTime5OID]);
$squidCacheHttpNmSvcTime5 = $squidCacheHttpNmSvcTime5_res->{$squidCacheHttpNmSvcTime5OID};

$squidCacheHttpNmSvcTime60OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.4.60' ;
$squidCacheHttpNmSvcTime60_res = $sess->get_request(-varbindlist =>[$squidCacheHttpNmSvcTime60OID]);
$squidCacheHttpNmSvcTime60 = $squidCacheHttpNmSvcTime60_res->{$squidCacheHttpNmSvcTime60OID};

if($squidCacheHttpNmSvcTime60 == 0)
{
   $squidCacheHttpNmSvcTime1To60 = 0;
}
else
{
   $squidCacheHttpNmSvcTime1To60 = $squidCacheHttpNmSvcTime1 / $squidCacheHttpNmSvcTime60;
}

#g)
$squidCacheHttpHitSvcTime1OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.5.1' ;
$squidCacheHttpHitSvcTime1_res = $sess->get_request(-varbindlist =>[$squidCacheHttpHitSvcTime1OID]);
$squidCacheHttpHitSvcTime1 = $squidCacheHttpHitSvcTime1_res->{$squidCacheHttpHitSvcTime1OID};

$squidCacheHttpHitSvcTime5OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.5.5' ;
$squidCacheHttpHitSvcTime5_res = $sess->get_request(-varbindlist =>[$squidCacheHttpHitSvcTime5OID]);
$squidCacheHttpHitSvcTime5 = $squidCacheHttpHitSvcTime5_res->{$squidCacheHttpHitSvcTime5OID};

$squidCacheHttpHitSvcTime60OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.5.60' ;
$squidCacheHttpHitSvcTime60_res = $sess->get_request(-varbindlist =>[$squidCacheHttpHitSvcTime60OID]);
$squidCacheHttpHitSvcTime60 = $squidCacheHttpHitSvcTime60_res->{$squidCacheHttpHitSvcTime60OID};

if ($squidCacheHttpHitSvcTime60 == 0)
{
   $squidCacheHttpHitSvcTime1To60 = 0;
}
else
{
   $squidCacheHttpHitSvcTime1To60 = $squidCacheHttpHitSvcTime1 / $squidCacheHttpHitSvcTime60;
}

#h)
$squidCacheDnsSvcTime1OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.8.1' ;
$squidCacheDnsSvcTime1_res = $sess->get_request(-varbindlist =>[$squidCacheDnsSvcTime1OID]);
$squidCacheDnsSvcTime1 = $squidCacheDnsSvcTime1_res->{$squidCacheDnsSvcTime1OID};

$squidCacheDnsSvcTime5OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.8.5' ;
$squidCacheDnsSvcTime5_res = $sess->get_request(-varbindlist =>[$squidCacheDnsSvcTime5OID]);
$squidCacheDnsSvcTime5 = $squidCacheDnsSvcTime5_res->{$squidCacheDnsSvcTime5OID};

$squidCacheDnsSvcTime60OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.8.60' ;
$squidCacheDnsSvcTime60_res = $sess->get_request(-varbindlist =>[$squidCacheDnsSvcTime60OID]);
$squidCacheDnsSvcTime60 = $squidCacheDnsSvcTime60_res->{$squidCacheDnsSvcTime60OID};

if($squidCacheDnsSvcTime60 == 0) 
{
   $squidCacheDnsSvcTime1To60 = 0;
}
else
{
   $squidCacheDnsSvcTime1To60 = $squidCacheDnsSvcTime1 / $squidCacheDnsSvcTime60;
}

#i)
$squidCacheRequestHitRatio1OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.9.1' ;
$squidCacheRequestHitRatio1_res = $sess->get_request(-varbindlist =>[$squidCacheRequestHitRatio1OID]);
$squidCacheRequestHitRatio1 = $squidCacheRequestHitRatio1_res->{$squidCacheRequestHitRatio1OID};

$squidCacheRequestHitRatio5OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.9.5' ;
$squidCacheRequestHitRatio5_res = $sess->get_request(-varbindlist =>[$squidCacheRequestHitRatio5OID]);
$squidCacheRequestHitRatio5 = $squidCacheRequestHitRatio5_res->{$squidCacheRequestHitRatio5OID};

$squidCacheRequestHitRatio60OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.9.60' ;
$squidCacheRequestHitRatio60_res = $sess->get_request(-varbindlist =>[$squidCacheRequestHitRatio60OID]);
$squidCacheRequestHitRatio60 = $squidCacheRequestHitRatio60_res->{$squidCacheRequestHitRatio60OID};

if($squidCacheRequestHitRatio60 == 0)
{
   $squidCacheRequestHitRatio1To60 = 0;
}
else
{
   $squidCacheRequestHitRatio1To60 = $squidCacheRequestHitRatio1 / $squidCacheRequestHitRatio60;
}

$s = Net::HTTP->new("127.0.0.1") or die "Can't connect: $@\n";
%header=("Accept","*");

#send resquest
$s->write_request("GET", $path , %header);

#recv respose header
($code, $mess, %headers) = $s->read_response_headers();
$count = $s->_rbuf_length ;

#recv response body 
$buf = "";
unlink "/tmp/sq_MgrMonHit.tmp";
while (1)
 {
       $n = $s->read_entity_body($buf, 1024);
       if($n<=0)
       {
            #print "***************bad response ,*************** \r\n";
            die "read failed: $!" unless defined $n;
            last unless $n;
       }

       #print $buf;
       open(targetFile, ">>/tmp/sq_MgrMonHit.tmp");
       @content = $buf;
       print targetFile @content;
       close(targetFile);
       $count = $count + $n;
}
@tmpFile = `cat /tmp/sq_MgrMonHit.tmp`;
foreach $line(@tmpFile)
{
   @cs = split(/ +/, $line);
   #if(@cs[0]=~/^\tRequest/)
   if(index($line, "Request") != -1 and index($line, "Memory") != -1)
   {
      $position1 = index($line, "5min:") + length("5min: ");
      $position2 = index($line, "%");
      $squidMemHitRatio5 = substr($line, $position1, $position2 - $position1);
      $position1 = index($line, "60min:") + length("60min: ");
      $position2 = index($line, "%", $position2+1);
      $squidMemHitRatio60 = substr($line, $position1, $position2 - $position1);
      if ($squidMemHitRatio60 == 0) 
      {
         $squidMemHitRatio5To60 = 0;
      }
      else
      {
         $squidMemHitRatio5To60 = $squidMemHitRatio5 / $squidMemHitRatio60;
      }
   }

    if(index($line, "Request") != -1 and index($line, "Disk") != -1)
   {
      $position1 = index($line, "5min:") + length("5min: ");
      $position2 = index($line, "%");
      $squidDiskHitRatio5 = substr($line, $position1, $position2 - $position1);
      $position1 = index($line, "60min:") + length("60min: ");
      $position2 = index($line, "%", $position2+1);
      $squidDiskHitRatio60 = substr($line, $position1, $position2 - $position1);
      if($squidDiskHitRatio60 == 0)
      {
         $squidDiskHitRatio5To60 = 0;
      }
      else
      {
         $squidDiskHitRatio5To60 = $squidDiskHitRatio5 / $squidDiskHitRatio60;
      }
   }
   
}

#j)
$squidCacheRequestByteRatio1OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.10.1' ;
$squidCacheRequestByteRatio1_res = $sess->get_request(-varbindlist =>[$squidCacheRequestByteRatio1OID]);
$squidCacheRequestByteRatio1 = $squidCacheRequestByteRatio1_res->{$squidCacheRequestByteRatio1OID};

$squidCacheRequestByteRatio5OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.10.5' ;
$squidCacheRequestByteRatio5_res = $sess->get_request(-varbindlist =>[$squidCacheRequestByteRatio5OID]);
$squidCacheRequestByteRatio5 = $squidCacheRequestByteRatio5_res->{$squidCacheRequestByteRatio5OID};

$squidCacheRequestByteRatio60OID = '.1.3.6.1.4.1.3495.1.3.2.2.1.10.60' ;
$squidCacheRequestByteRatio60_res = $sess->get_request(-varbindlist =>[$squidCacheRequestByteRatio60OID]);
$squidCacheRequestByteRatio60 = $squidCacheRequestByteRatio60_res->{$squidCacheRequestByteRatio60OID};

if($squidCacheRequestByteRatio60 == 0)
{
   $squidCacheRequestByteRatio1To60 = 0;
}
else
{
   $squidCacheRequestByteRatio1To60 = $squidCacheRequestByteRatio1 / $squidCacheRequestByteRatio60;
}

$t=time();

#print "squidCacheSysVMsize:   ", $squidCacheSysVMsize , "\r\n" ;

#save to the pool file
if(!($squidCacheSysVMsize cmp ""))
   {
      $squidCacheSysVMsize = -1;
   }
$cmdStr1 = sprintf("echo squidCacheSysVMsize: %s > ../pool/squidMgrMon.pool", $squidCacheSysVMsize);

if(!($squidCacheSysStorage cmp ""))
   {
      $squidCacheSysStorage = -1;
   }
$cmdStr2 = sprintf("echo squidCacheSysStorage: %s >> ../pool/squidMgrMon.pool", $squidCacheSysStorage);

if(!($squidCacheMemUsage cmp ""))
   {
      $squidCacheMemUsage = -1;
   }
$cmdStr3 = sprintf("echo squidCacheMemUsage: %s >> ../pool/squidMgrMon.pool", $squidCacheMemUsage);

if(!($squidCacheHttpErrors cmp ""))
   {
      $squidCacheHttpErrors = -1;
   }
$cmdStr4 = sprintf("echo squidCacheHttpErrors: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpErrors);

if(!($squidCacheHttpErrorsIncreased cmp ""))
   {
      $squidCacheHttpErrorsIncreased = -1;
   }
$cmdStr5 = sprintf("echo squidCacheHttpErrorsIncreased: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpErrorsIncreased);

if(!($squidCacheHttpMissSvcTime1 cmp ""))
   {
      $squidCacheHttpMissSvcTime1 = -1;
   }
$cmdStr6 = sprintf("echo squidCacheHttpMissSvcTime1: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpMissSvcTime1);

if(!($squidCacheHttpMissSvcTime5 cmp ""))
   {
      $squidCacheHttpMissSvcTime5 = -1;
   }
$cmdStr7 = sprintf("echo squidCacheHttpMissSvcTime5: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpMissSvcTime5);

if(!($squidCacheHttpMissSvcTime60 cmp ""))
   {
      $squidCacheHttpMissSvcTime60 = -1;
   }
$cmdStr8 = sprintf("echo squidCacheHttpMissSvcTime60: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpMissSvcTime60);

if(!($squidCacheHttpMissSvcTime1To60 cmp ""))
   {
      $squidCacheHttpMissSvcTime1To60 = -1;
   }
$cmdStr9 = sprintf("echo squidCacheHttpMissSvcTime1To60: %0.2f >> ../pool/squidMgrMon.pool", $squidCacheHttpMissSvcTime1To60);

if(!($squidCacheHttpNmSvcTime1 cmp ""))
   {
      $squidCacheHttpNmSvcTime1 = -1;
   }
$cmdStr10 = sprintf("echo squidCacheHttpNmSvcTime1: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpNmSvcTime1);

if(!($squidCacheHttpNmSvcTime5 cmp ""))
   {
      $squidCacheHttpNmSvcTime5 = -1;
   }
$cmdStr11 = sprintf("echo squidCacheHttpNmSvcTime5: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpNmSvcTime5);

if(!($squidCacheHttpNmSvcTime60 cmp ""))
   {
      $squidCacheHttpNmSvcTime60 = -1;
   }
$cmdStr12 = sprintf("echo squidCacheHttpNmSvcTime60: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpNmSvcTime60);

if(!($squidCacheHttpNmSvcTime1To60 cmp ""))
   {
      $squidCacheHttpNmSvcTime1To60 = -1;
   }
$cmdStr13 = sprintf("echo squidCacheHttpNmSvcTime1To60: %0.2f >> ../pool/squidMgrMon.pool", $squidCacheHttpNmSvcTime1To60);

if(!($squidCacheHttpHitSvcTime1 cmp ""))
   {
      $squidCacheHttpHitSvcTime1 = -1;
   }
$cmdStr14 = sprintf("echo squidCacheHttpHitSvcTime1: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpHitSvcTime1);

if(!($squidCacheHttpHitSvcTime5 cmp ""))
   {
      $squidCacheHttpHitSvcTime5 = -1;
   }
$cmdStr15 = sprintf("echo squidCacheHttpHitSvcTime5: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpHitSvcTime5);

if(!($squidCacheHttpHitSvcTime60 cmp ""))
   {
      $squidCacheHttpHitSvcTime60 = -1;
   }
$cmdStr16 = sprintf("echo squidCacheHttpHitSvcTime60: %s >> ../pool/squidMgrMon.pool", $squidCacheHttpHitSvcTime60);

if(!($squidCacheHttpHitSvcTime1To60 cmp ""))
   {
      $squidCacheHttpHitSvcTime1To60 = -1;
   }
$cmdStr17 = sprintf("echo squidCacheHttpHitSvcTime1To60: %0.2f >> ../pool/squidMgrMon.pool", $squidCacheHttpHitSvcTime1To60);

if(!($squidCacheDnsSvcTime1 cmp ""))
   {
      $squidCacheDnsSvcTime1 = -1;
   }
$cmdStr18 = sprintf("echo squidCacheDnsSvcTime1: %s >> ../pool/squidMgrMon.pool", $squidCacheDnsSvcTime1);

if(!($squidCacheDnsSvcTime5 cmp ""))
   {
      $squidCacheDnsSvcTime5 = -1;
   }
$cmdStr19 = sprintf("echo squidCacheDnsSvcTime5: %s >> ../pool/squidMgrMon.pool", $squidCacheDnsSvcTime5);

if(!($squidCacheDnsSvcTime60 cmp ""))
   {
      $squidCacheDnsSvcTime60 = -1;
   }
$cmdStr20 = sprintf("echo squidCacheDnsSvcTime60: %s >> ../pool/squidMgrMon.pool", $squidCacheDnsSvcTime60);

if(!($squidCacheDnsSvcTime1To60 cmp ""))
   {
      $squidCacheDnsSvcTime1To60 = -1;
   }
$cmdStr21 = sprintf("echo squidCacheDnsSvcTime1To60: %0.2f >> ../pool/squidMgrMon.pool", $squidCacheDnsSvcTime1To60);

if(!($squidCacheRequestHitRatio1 cmp ""))
   {
      $squidCacheRequestHitRatio1 = -1;
   }
$cmdStr22 = sprintf("echo squidCacheRequestHitRatio1: %s >> ../pool/squidMgrMon.pool", $squidCacheRequestHitRatio1);

if(!($squidCacheRequestHitRatio5 cmp ""))
   {
      $squidCacheRequestHitRatio5 = -1;
   }
$cmdStr23 = sprintf("echo squidCacheRequestHitRatio5: %s >> ../pool/squidMgrMon.pool", $squidCacheRequestHitRatio5);

if(!($squidCacheRequestHitRatio60 cmp ""))
   {
      $squidCacheRequestHitRatio60 = -1;
   }
$cmdStr24 = sprintf("echo squidCacheRequestHitRatio60: %s >> ../pool/squidMgrMon.pool", $squidCacheRequestHitRatio60);

if(!($squidCacheRequestHitRatio1To60 cmp ""))
   {
      $squidCacheRequestHitRatio1To60 = -1;
   }
$cmdStr25 = sprintf("echo squidCacheRequestHitRatio1To60: %0.2f >> ../pool/squidMgrMon.pool", $squidCacheRequestHitRatio1To60);

if(!($squidMemHitRatio5 cmp ""))
   {
      $squidMemHitRatio5 = -1;
   }
$cmdStr26 = sprintf("echo squidMemHitRatio5: %s >> ../pool/squidMgrMon.pool", $squidMemHitRatio5);

if(!($squidMemHitRatio60 cmp ""))
   {
      $squidMemHitRatio60 = -1;
   }
$cmdStr27 = sprintf("echo squidMemHitRatio60: %s >> ../pool/squidMgrMon.pool", $squidMemHitRatio60);

if(!($squidMemHitRatio5To60 cmp ""))
   {
      $squidMemHitRatio5To60 = -1;
   }
$cmdStr28 = sprintf("echo squidMemHitRatio5To60: %0.2f >> ../pool/squidMgrMon.pool", $squidMemHitRatio5To60);

if(!($squidDiskHitRatio5 cmp ""))
   {
      $squidDiskHitRatio5 = -1;
   }
$cmdStr29 = sprintf("echo squidDiskHitRatio5: %s >> ../pool/squidMgrMon.pool", $squidDiskHitRatio5);

if(!($squidDiskHitRatio60 cmp ""))
   {
      $squidDiskHitRatio60 = -1;
   }
$cmdStr30 = sprintf("echo squidDiskHitRatio60: %s >> ../pool/squidMgrMon.pool", $squidDiskHitRatio60);

if(!($squidDiskHitRatio5To60 cmp ""))
   {
      $squidDiskHitRatio5To60 = -1;
   }
$cmdStr31 = sprintf("echo squidDiskHitRatio5To60: %0.2f >> ../pool/squidMgrMon.pool", $squidDiskHitRatio5To60);

if(!($squidCacheRequestByteRatio1 cmp ""))
   {
      $squidCacheRequestByteRatio1 = -1;
   }
$cmdStr32 = sprintf("echo squidCacheRequestByteRatio1: %s >> ../pool/squidMgrMon.pool", $squidCacheRequestByteRatio1);

if(!($squidCacheRequestByteRatio5 cmp ""))
   {
      $squidCacheRequestByteRatio5 = -1;
   }
$cmdStr33 = sprintf("echo squidCacheRequestByteRatio5: %s >> ../pool/squidMgrMon.pool", $squidCacheRequestByteRatio5);

if(!($squidCacheRequestByteRatio60 cmp ""))
   {
      $squidCacheRequestByteRatio60 = -1;
   }
$cmdStr34 = sprintf("echo squidCacheRequestByteRatio60: %s >> ../pool/squidMgrMon.pool", $squidCacheRequestByteRatio60);

if(!($squidCacheRequestByteRatio1To60 cmp ""))
   {
      $squidCacheRequestByteRatio1To60 = -1;
   }
$cmdStr35 = sprintf("echo squidCacheRequestByteRatio1To60: %0.2f >> ../pool/squidMgrMon.pool", $squidCacheRequestByteRatio1To60);

$cmdStr36 = sprintf("echo squidMgrChkUpdateTime: %s >> ../pool/squidMgrMon.pool", $t);

system($cmdStr1);
system($cmdStr2);
system($cmdStr3);
system($cmdStr4);
system($cmdStr5);
system($cmdStr6);
system($cmdStr7);
system($cmdStr8);
system($cmdStr9);
system($cmdStr10);
system($cmdStr11);
system($cmdStr12);
system($cmdStr13);
system($cmdStr14);
system($cmdStr15);
system($cmdStr16);
system($cmdStr17);
system($cmdStr18);
system($cmdStr19);
system($cmdStr20);
system($cmdStr21);
system($cmdStr22);
system($cmdStr23);
system($cmdStr24);
system($cmdStr25);
system($cmdStr26);
system($cmdStr27);
system($cmdStr28);
system($cmdStr29);
system($cmdStr30);
system($cmdStr31);
system($cmdStr32);
system($cmdStr33);
system($cmdStr34);
system($cmdStr35);
system($cmdStr36);
system($cmdSaveErrors);

exit ;
