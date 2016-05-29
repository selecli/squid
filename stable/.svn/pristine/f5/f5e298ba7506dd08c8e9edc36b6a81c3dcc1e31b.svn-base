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
      print "squidConnMon Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidConnMon -v\" for version number.", "\r\n";
      print "Use \"squidConnMon-h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidConnMon -h\" for help.", "\r\n";
      exit;
   }
}

$str = "Time:1222:12:13:14  sdf ";

@t = split(/:+/, $str);

$count =0;
$i = 0;
$j = 0;
$k=0;

$localCount =0;
$remoteCount = 0;
$netCount = 0;

sub  connCount
{

 ($line , $localPort , $remotePort) = @_ ;

 
 @dots = split(/ +/,$line) ;
 
 $i =0;
 
 $flag =0 ;  # 1 for local stat , 2 for  remote stat ;
    
   foreach $content(@dots)
   {
    $i= $i+1;

    next  if($i <= 2) ;

    if($i==3) #local machine info
     {

      @add_port =split(/:/, $content);

      $k=0 ;
      foreach $each(@add_port)
       {
          $k=$k+1;

          #next if($k<=1) ;

          $each = $each +0;
          if($each== $localPort)
          {
               $flag =1 ;
          }

       }
     }

     if($i==4) #remote machine info
     {

      @add_port =split(/:/, $content);

      foreach $each(@add_port)
       {

          #next if($k<=1) ;

          $each = $each +0;
          if($each== $remotePort)
          {
               $flag =2 ;
          }         
       }
     }

     if($i ==5) #connection state
      {
         $state =  $content +0 ;

         if($state == 1)
           {
            if($flag ==1)
            {
               $localCount ++;
            }  
            elsif($flag ==2)
             { 
               $remoteCount ++;
             }
              
            #system net connection count
            $netCount ++ ;           
      
           }
      }

  }
#print "\r\n" ; 
}



open(MY_FILE , "/proc/net/tcp") || die "open file failed $!" ;

@eachLine =<MY_FILE>;

close (MY_FILE);

$lineNum =0;

foreach $line(@eachLine)
{
  $lineNum = $lineNum +1 ;
 
   next if($lineNum<2) ;
 
   $localPort =50 ;
   $remotePort = 50 ;
   connCount($line,$localPort , $remotePort);

}




$lineNum =0;

open(MY_FILE , "/proc/net/tcp6") || die "open file failed $!" ;

@eachLine =<MY_FILE>;

close (MY_FILE);

foreach $line(@eachLine)
{
  $lineNum = $lineNum +1 ;

  next if($lineNum<2) ;
   
   $localPort =50 ;
    $remotePort = 50 ;
    connCount($line,$localPort , $remotePort);
}

print "client: " , $localCount , "\tserver: ", $remoteCount , "\r\n";


$strCmd=sprintf("echo squidClientConnCount: %s > ../pool/squidConnMon.pool", $localCount);
system($strCmd);

$strCmd=sprintf("echo squidHttpServerConnCount: %s >> ../pool/squidConnMon.pool", $remoteCount);
system($strCmd);

$lt = time();
$strCmd = sprintf("echo squidConnUpdateTime: %s >> ../pool/squidConnMon.pool", $lt);
system($strCmd);


#system net connection count
#$strCmd=sprintf("echo %s > ../pool/netConnMon.pool", $netCount);
#system($strCmd);

#$lt = time();
#$strCmd = sprintf("echo %s >> ../pool/netConnMon.pool", $lt);
#system($strCmd);

exit ;

