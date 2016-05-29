#!/usr/bin/perl

#********************************
#Desc:   read disk monitor'data
#Author: Qingrong.Jiang
#Date :  2006-03-09
#********************************


use POSIX;
use POSIX qw(setsid);
use POSIX qw(:errno_h :fcntl_h);

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "diskReader Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"diskReader -v\" for version number.", "\r\n";
      print "Use \"diskReader -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"diskReader -h\" for help.", "\r\n";
      exit;
   }
}

#set cwd
$cmd1 = "cd /monitor/bin/" ;
$cmd2 = "/usr/bin/perl /monitor/bin/diskSpeedReader.pl";
$cmd3 = "/usr/bin/perl /monitor/bin/diskSpaceReader.pl";

#execute command
system($cmd1);
system($cmd2);
system($cmd3);

