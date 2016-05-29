#!/usr/bin/perl

#********************************
#Desc:   read net monitor'data
#Author: Qingrong.Jiang
#Date :  2006-03-15
#********************************


use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "netReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"netReader -v\" for version number.", "\r\n";
      print "Use \"netReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"netReader -h\" for help.", "\r\n";
      exit;
   }
}

#set cwd
$cmd1 = "cd /monitor/bin/" ;
$cmd2 = "/usr/bin/perl /monitor/bin/netConnReader.pl";
$cmd3 = "/usr/bin/perl /monitor/bin/netBandwidthReader.pl";
$cmd4 = "/usr/bin/perl /monitor/bin/netStatusReader.pl";
#execute command
system($cmd1);
system($cmd2);
system($cmd3);
system($cmd4);

