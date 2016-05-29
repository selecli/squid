#!/usr/bin/perl

#********************************
#Desc:   check squid response speed
#Author: Qingrong.Jiang
#Date :  2006-03-06
#********************************


use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "loadMon Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"loadMon -v\" for version number.", "\r\n";
      print "Use \"loadMon -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"loadMon -h\" for help.", "\r\n";
      exit;
   }
}

system("../bin/loadMon.sh");

$lt = time();

$strCmd = sprintf("echo loadUpdateTime: %s >> ../pool/loadMon.pool", $lt);

system($strCmd);
