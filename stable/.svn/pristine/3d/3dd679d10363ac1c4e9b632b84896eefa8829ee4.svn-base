#!/usr/bin/perl

#********************************
#Desc:   read squid monitor'data
#Author: Qingrong.Jiang
#Date :  2006-03-09
#********************************


use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.4;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "squidReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidReader -v\" for version number.", "\r\n";
      print "Use \"squidReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidReader -h\" for help.", "\r\n";
      exit;
   }
}

$cmd1 = "cd /monitor/bin/" ;
$cmd2 = "/usr/bin/perl /monitor/bin/squidConnReader.pl";
$cmd3 = "/usr/bin/perl /monitor/bin/squidResponseReader.pl";
$cmd4 = "/usr/bin/perl /monitor/bin/squidOriginResponseReader.pl";
$cmd5 = "/usr/bin/perl /monitor/bin/squidBandwidthReader.pl";
$cmd6 = "/usr/bin/perl /monitor/bin/squidLogTransReader.pl";
$cmd7 = "/usr/bin/perl /monitor/bin/squidMgrReader.pl";
$cmd8 = "/usr/bin/perl /monitor/bin/origMonReader.sh";
#added for monitor purgeGetopt.sh  .1.3.6.1.4.1.25337.30.1.101.57
$cmd9 = "/bin/bash /monitor/bin/purgeGetoptMonReader.sh";
#add for squid version control .1.3.6.1.4.1.25337.30.1.101.58/.1.3.6.1.4.1.25337.30.1.101.59
$cmd10 = "/bin/bash /monitor/bin/squidVersionReader.sh";
#added for dig monitor  1.101.60/1.101.61/1.101.62 
$cmd11 = "/bin/bash /monitor/bin/digMonReader.sh";
#added for diskStatus monitor  1.101.63/1.101.64 
$cmd12 = "/bin/bash /monitor/bin/diskStatusReader.sh";
#monitor squid numbers of logs in archive dir 1.101.65
$cmd13 = "/bin/bash /monitor/bin/squidLogUploadReader.sh";
#added for swap.state file monitor 1.101.66/1.101.67 
$cmd14 = "/bin/bash /monitor/bin/monReader.sh -m squidSwapState.2";
#added for billing monitor, agent at /usr/local/squid/bin/rotateBilling.sh 1.101.68 
$cmd15 = "/bin/bash /monitor/bin/monReader.sh -m squidBilling.1";
#added for log upload status monitor
#execute command
system($cmd1);
system($cmd2);
system($cmd3);
system($cmd4);
system($cmd5);
system($cmd6);
system($cmd7);
system($cmd8);
#added for monitor purgeGetopt.sh
system($cmd9);
#add for squid version control
system($cmd10);
#added for dig monitor
system($cmd11);
#added for diskStatus monitor
system($cmd12);
#added for log upload status monitor
system($cmd13);
#added for swap.state file monitor
system($cmd14);
#added for billing monitor
system($cmd15);
