#!/bin/bash
#Rewrote by YangNan at 2009-03-27
DEBUG=1
ConfDir=/usr/local/squid/etc
Conf=${ConfDir}/squid.conf
Key=/usr/local/squid/etc/key.conf
KeyTmp=/usr/local/squid/var/.key.tmp
Verify=/usr/local/squid/var/keyverify
Red="\033[0;31;1m"
Green="\033[0;32;1m"
Comeback="\033[0m"
Server1=61.135.204.241
Server2=220.181.39.241
#--------------------------------------------------------------------
if [ -e "$Verify" -o ! -e "$Key" -o ! -s "$Key" ]
 then
  nmap -sT -p 80 $Server1|grep open>/dev/null
  Server1Status=$?
  nmap -sT -p 80 $Server2|grep open>/dev/null
  Server2Status=$?
  if [ "$Server1Status" -eq 0 ]
   then
    ActiveServer=$Server1
    echo -e $Green"Authentification server $Server1 is active"$Comeback
   elif [ "$Server2Status" -eq 0 ]
    then
     ActiveServer=$Server2
     echo -e $Green"Authentification server $Server2 is active"$Comeback
   else
    echo -e $Red"All of the authentification servers are inactive"$Comeback
    exit 1
  fi
  PostTo=http://$ActiveServer/squid_admin.php
  echo Using $PostTo as the destination
  Core=$(/sbin/ifconfig -a|awk '$4 ~ /HWaddr/ {print $5}'|uniq)
  [ "$DEBUG" -ge 2 ]&&echo "$Core"
  wget  --post-data value="$Core" $PostTo -O $KeyTmp >/dev/null 2>&1
  mv -f $KeyTmp $Key
  InitKey=$([ -e "$Key" ]&&grep SQUID_KEY $Key)
  [ "$DEBUG" -ge 1 ]&&echo -e "Initiating Key with value:\n$InitKey"
 else
  InitKey=$([ -e "$Key" ]&&grep SQUID_KEY $Key)
  [ "$DEBUG" -ge 1 ]&&echo -e "Existing key with value:\n$InitKey"
fi
