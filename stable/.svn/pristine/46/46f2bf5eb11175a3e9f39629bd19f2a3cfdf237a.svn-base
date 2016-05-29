#!/usr/bin/perl

#******************************************************
#Desc:   check squid client , server bandwidth .
#Author: Qingrong.Jiang
#Date :  2006-03-09
#******************************************************


use Net::SNMP;

$version = 1.3;
if(@ARGV > 0)
{
   if(@ARGV == 1 and !($ARGV[0] cmp "-v"))
   {
      print "squidBandwidthMon Version: ", $version, "\r\n";
      exit;
   }
   elsif(@ARGV ==1 and !($ARGV[0] cmp "-h"))
   {
      print "Use \"squidBandwidthMon -v\" for version number.", "\r\n";
      print "Use \"squidBandwidthMon -h\" for help.", "\r\n";
      exit;
   }
   else
   {
      print "error argument.", "\r\n";
      print "Use \"squidBandwidthMon -h\" for help.", "\r\n";
      exit;
   }
}

$hostname='127.0.0.1';
my $port='3401';
my $community='public';

$SNMP::debugging = 0;
$SNMP::dump_packet = 0;
my ($sess, $error) = Net::SNMP->session(
   -hostname  => shift || '127.0.0.1',
   -community => shift || 'public',
   -port      => shift || 3401
);

#$sess = new SNMP::Session( 'DestHost'   => $hostname,
#                           'Community'  => $community,
#                           'RemotePort' => $port,
#                           'Timeout'    => 300000,
#                           'Retries'    => 3,
#                           'Version'    => '2c',
#                           'UseLongNames' => 1,    # Return full OID tags
#                           'UseNumeric' => 1,      # Return dotted decimal OID
#                           'UseEnums'   => 0,      # Don't use enumerated vals
#                           'UseSprintValue' => 0); # Don't pretty-print values

#get data that client sents to squid
$clientInDataOID= '.1.3.6.1.4.1.3495.1.3.2.1.4' ;
$clientInKb_res = $sess->get_request(-varbindlist =>[$clientInDataOID]);
$clientInKb = $clientInKb_res->{$clientInDataOID} ;


#get data that squid sents to client
$clientOutDataOID = '.1.3.6.1.4.1.3495.1.3.2.1.5' ;
$clientOutKb_res = $sess->get_request(-varbindlist =>[$clientOutDataOID]);
$clientOutKb = $clientOutKb_res->{$clientOutDataOID} ;


#get data that server sents to squid
$serverInDataOID = '.1.3.6.1.4.1.3495.1.3.2.1.12' ;
$serverInKb_res = $sess->get_request(-varbindlist =>[$serverInDataOID]);
$serverInKb = $serverInKb_res->{$serverInDataOID} ;

#get  data that squid sents to server
$serverOutDataOID = '.1.3.6.1.4.1.3495.1.3.2.1.13' ;
$serverOutKb_res = $sess->get_request(-varbindlist =>[$serverOutDataOID]);
$serverOutKb = $serverOutKb_res->{$serverOutDataOID};


$t=time();

#format data
$ff = sprintf("%s\t%s\t%s\t%s\t%s", $clientInKb, $clientOutKb , $serverInKb ,$serverOutKb, $t);

$cmd1 = sprintf("echo %s > /tmp/sq_bandwidth.tmp", $ff) ;

#get stale data from tmp file , /tmp/sq_bandwidth.tmp
@fileData=`cat /tmp/sq_bandwidth.tmp` ;


$line =  @fileData[0] ;
  
@data= split(/ +/, $line);
$s_clientInKb = @data[0];
$s_clientOutKb = @data[1];
$s_serverInKb = @data[2];
$s_serverOutKb = @data[3];
$s_t = @data[4];

#caculate net band width ;
$clientInBandwidth =0;
$clientOutBandwidth =0;
$serverInBandwidth =0;
$serverOutBandwidth =0;

$t_diff = $t -$s_t ;
if($t_diff>0 and $t_diff<300)
{
   if($clientInKb>0 and $s_clientInKb >0 and $clientInKb > $s_clientInKb)
    {
       $clientInBandwidth =($clientInKb - $s_clientInKb)/$t_diff ;
    }      

   if($clientOutKb>0 and $s_clientOutKb >0 and $clientOutKb > $s_clientOutKb)
    {
       $clientOutBandwidth =($clientOutKb - $s_clientOutKb)/$t_diff ;
    }

   if($serverInKb>0 and $s_serverInKb >0 and $serverInKb > $s_serverInKb)
    {
       $serverInBandwidth =($serverInKb - $s_serverInKb)/$t_diff ;
    }

   if($serverOutKb>0 and $s_serverOutKb >0 and $serverOutKb > $s_serverOutKb)
    {
       $serverOutBandwidth =($serverOutKb - $s_serverOutKb)/$t_diff ;
    }


}

print "client In : " , $clientInKb ,    " - " , $s_clientInKb , " = " , ($clientInKb - $s_clientInKb) , "\r\n" ;
print "client Out: " , $clientOutKb ,   " - " , $s_clientOutKb ," = " , ($clientOutKb - $s_clientOutKb) , "\r\n" ;
print "server In : " , $serverInKb ,    " - " , $s_serverInKb , " = " , ($serverInKb - $s_serverInKb) , "\r\n" ;
print "server Out: " , $serverOutKb ,   " - " , $s_serverOutKb , " = " , ($serverOutKb - $s_serverOutKb) , "\r\n" ;
print "time interval : " , $t_diff , "\r\n";

print "clientInBandwidth:   ", $clientInBandwidth , "\r\n" ;
print "clientOutBandwidth:  ", $clientOutBandwidth , "\r\n" ;
print "serverInBandwidth:   ", $serverInBandwidth , "\r\n" ;
print "serverOutBandwidth:  ", $serverOutBandwidth , "\r\n" ;

$cmdStr1 = sprintf("echo squidClientInBandwidthInKBps: %0.2f > ../pool/squidBandwidthMon.pool", $clientInBandwidth);
$cmdStr2 = sprintf("echo squidClinetOutBandwidthInKBps: %0.2f >> ../pool/squidBandwidthMon.pool", $clientOutBandwidth);
$cmdStr3 = sprintf("echo squidServerInBandwidthinKBps: %0.2f >> ../pool/squidBandwidthMon.pool", $serverInBandwidth);
$cmdStr4 = sprintf("echo squidServerOutBandwidthInKBps: %0.2f >> ../pool/squidBandwidthMon.pool", $serverOutBandwidth);
$cmdStr5 = sprintf("echo squidBandwidthUpdateTime: %s >> ../pool/squidBandwidthMon.pool", $t);
system($cmdStr1);
system($cmdStr2);
system($cmdStr3);
system($cmdStr4);
system($cmdStr5);
system($cmd1);

 


print $ss , "\r\n" ;

exit ;
