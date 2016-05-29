#!/bin/bash

# Chinacache R&D FlexiCache Team 
# $Id: rotate.sh 1045 2007-06-28 07:12:46Z alex $

#---------------------------------------------------------------------------|
#  @programe  : rotateReceipt.sh                                            |  
#  @version   : 0.0.3                                                       |
#  @function@ : rotate receipt log                                          |
#  @campany   : china cache                                                 |
#  @dep.      : R&D FlexiCache Team                                         |
#  @writer    : yuanpeng.jiang                                              |
#---------------------------------------------------------------------------|
. /usr/local/squid/bin/functions 
tryLockProcess $0
if [ $? -ne 0 ] ;then exit 0;fi     

export PATH=.:/usr/local/squid/sbin:/usr/local/squid/bin:/usr/kerberos/sbin:/usr/kerberos/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin:/root/bin
DEBUG=1
#set -e

#You must tell this script where the configuration file is.

def_conf=/usr/local/squid/etc/upload.conf

Version( )
{
        echo     "      `basename $0` 0.0.3 
                        Last modified at 08/07/30                       
                        Written by yuanpeng.jiang
                        EmailTo:   yuanpeng.jiang@chinacache.com
                        Copyright (C) ChinaCache Co.,Ltd."
}
Help( )
{
        echo Example:
        echo     "      `basename $0` -h ";
        echo     "      `basename $0` -v ";
        echo     "      `basename $0`";
}


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

[ -f $def_conf ] || \
{
echo "Wanning: /usr/local/squid/etc/upload.conf does not exist!";
echo " Programe aborted! ";
unlockProcess $0
exit 1
}


#you can change your BoxID:(like hostname)
#defDevID=0100000_notconfig
if [ -e /sn.txt ]
then
	Sn=`cat /sn.txt`
	Node=`hostname`
	defDevID=$Sn'_'${Node%-*}
else
	Node=`hostname`
	defDevID=0100000300_${Node%-*}
fi 

defLogBaseDir=/data/proclog/log/squid


ifDevID=$(cat $def_conf|cut -d '#' -f1|awk -F '=' '$1 ~ /DevID/ {print $2}'| \
        awk '{print $1}'|tail -n1 )

DevID=${ifDevID:-$defDevID}
Ldir=${defLogBaseDir}
Backdir=${defLogBaseDir}/backup
ifB_LogBaseDir=$(cat $def_conf|cut -d '#' -f1 \
	|awk -F '=' '$1 ~ /Microsoft_LogBaseDir/ {print $2}'|awk '{print $1}'|tail -n1)

#---------final config --------------------------------
B_DevID=$DevID
B_LogBaseDir=${defLogBaseDir}
B_Ldir=${B_LogBaseDir}/Receipt
test -d $B_Ldir ||mkdir -p $B_Ldir
cd $Ldir
#---------rename -------------------------

#---------test if "receipt*.log*" exists----------
exist=`/usr/bin/find $B_LogBaseDir -maxdepth 1  -type f -name 'receipt*.log*' -mmin +0 -print`
if [ -z $exist ]
then
	[ $DEBUG -ge 1 ] && echo "exit for not exist receipt*.log*"
        unlockProcess $0
	exit
fi
#---------test end---------

mydate=`date "+%m%d%H%M%S.%N"`
> ${B_Ldir}/${B_DevID}_receipt${mydate}.log

while read Receipt;do
[ $DEBUG -ge 1 ] &&echo $Receipt

cat $Receipt >>${B_Ldir}/${B_DevID}_receipt${mydate}.log
rm -f $Receipt
done < <(/usr/bin/find $B_LogBaseDir -maxdepth 1 -type f -name 'receipt*.log*' -mmin +0 -print)
#echo '=====================End of new ======================================'
unlockProcess $0
