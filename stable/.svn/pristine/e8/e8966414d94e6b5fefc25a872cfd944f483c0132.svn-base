#!/bin/bash

# Chinacache R&D Dept. FlexiCache Team  
# $Id: dellog.sh 4877 2008-08-20 05:24:51Z jyp $
#---------------------------------------------------------------------------|
#  @program   : dellog.sh                                                   |  
#  @version   : 2.0.6                                                       |
#  @function@ : to delete squid logs        .                               |
#  @campany   : china cache                                                 |
#  @dep.      : Chinacache R&D Dept. FlexiCache Team                        |
#  @writer    : Alex  Yu & yuanpeng.jiang                                   |
#---------------------------------------------------------------------------|
. /usr/local/squid/bin/functions 
tryLockProcess $0
if [ $? -ne 0 ] ;then exit 0;fi     

export PATH=.:/usr/local/squid/sbin:/usr/local/squid/bin:/usr/kerberos/sbin:/usr/kerberos/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin:/root/bin
DEBUG=0
NO_ARGS=0
E_OPTERROR=65
ConfFile=/usr/local/squid/etc/fcexternal.conf
#ConfTag='#*->'

opt1=$1

MinSize=380
DefSize=380000
MinMinutes=30
DefMinutes=1440
DefMinutes_2=10080
#--------------------------------------------------------------------------
#dellog_size 36000K
#*-> #purge_delay on/off
#*-> dellog_size 36000K
#*-> dellog_minutes


while read Entry Value Others
do
	if [ "$Entry" = "dellog_size"  ];then
                [ "$DEBUG" -ge "1" ]&&echo "confEntry :$Entry $Value"
                if [ "$Value" -ge "$MinSize" ];then
                        dellog_size=$Value
                        [ "$DEBUG" -ge "1" ]&&echo "dellog_size=$dellog_size"
                fi
        fi

        if [ "$Entry" = "dellog_minutes"  ];then
                [ "$DEBUG" -ge "1" ]&&echo "confEntry :$Entry $Value"
                if [ "$Value" -ge "$MinMinutes" ];then
                        dellog_minutes=$Value
                        [ "$DEBUG" -ge "1" ]&&echo "dellog_minutes=$dellog_minutes"
                fi
        fi

done < <(test -r $ConfFile&&grep -v '^#' $ConfFile|grep -v '^$')


DellogSize=${dellog_size:-${DefSize}}
DellogMinutes=${dellog_minutes:-${DefMinutes}}

[ "$DEBUG" -ge "1" ] \
      &&echo "results :$_ConfTag  DellogSize=$DellogSize;DellogMinutes=$DellogMinutes"

##################################################################################
##################################################################################
Help( )
{
        echo Example:
        echo     "      `basename $0`    "; 
        echo     "      `basename $0` -h ";
        echo     "      `basename $0` -v ";
}
Version( )
{
        echo     "      `basename $0` 2.0.6 
                        Last modified at 2007-01-19                       
                        Written by Alex Yu
			Modified by yuanpeng.jiang 2009/12/15
                        EmailTo:   yuanpeng.jiang@chinacache.com
                        Copyright (C) ChinaCache Co.,Ltd."
}
def_conf=/usr/local/squid/etc/upload.conf

ifLogBaseDir=$(cat $def_conf|cut -d '#' -f1|awk -F '=' '$1 ~ /LogBaseDir/ {print $2}'| \
        awk '{print $1}'|tail -n1)
defLogBaseDir=/data/proclog/log/squid
LogBaseDir=${ifLogBaseDir:-$defLogBaseDir}
Ldir=${LogBaseDir}/archive
Backdir=${LogBaseDir}/backup
test -d $Backdir ||mkdir -p $Backdir
if [ "$1" == "-v" ] ;then
Version
unlockProcess $0
exit 0
fi

if [ "$1" == "-h" ] ;then
Help
unlockProcess $0
exit 0
fi
FIND="/usr/bin/find"
$FIND /var/log/chinacache/* ! -name "*_cache_*.log" \
	-size +${DellogSize}k -exec rm -f {} \;
$FIND /var/log/chinacache/* -name "*_cache_*.log" \
	-cmin +${DefMinutes_2} -exec rm -f {} \;
$FIND /var/log/chinacache/* -name "*.log" \
        -cmin +${DefMinutes_2} -exec rm -f {} \;
unlockProcess $0
