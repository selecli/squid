#!/bin/bash

# Chinacache R&D FlexiCache Team 
# $Id: rotate.sh 1045 2007-06-28 07:12:46Z alex $
#---------------------------------------------------------------------------|
#  @programe  : rotateAccess.sh                                             |  
#  @version   : 0.0.5                                                       |
#  @function@ : rotate squid access log                                     |
#  @campany   : china cache                                                 |
#  @dep.      : R&D FlexiCache Team                                         |
#  @writer    : Alex  Yu & yuanpeng.jiang                                   |
#---------------------------------------------------------------------------|
export PATH=.:/usr/local/squid/sbin:/usr/local/squid/bin:/usr/kerberos/sbin:/usr/kerberos/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin:/root/bin
#set -e

#You must tell this script where the configuration file is.

def_conf=/usr/local/squid/etc/upload.conf
Version( )
{
        echo     "      `basename $0` 0.0.5 
                        Last modified at 09/12/16                       
                        Written by Alex Yu
			Modified by yuanpeng.jiang 08/12/16
			Modified by yuanpeng.jiang 09/12/15
			Modified by yuanpeng.jiang 09/12/18
                        EmailTo:   yuanpeng.jiang@chinacache.com
                        Copyright (C) ChinaCache Co.,Ltd."
}
Help( )
{
        echo Example:
        echo     "      `basename $0` -h ";
        echo     "      `basename $0` -v ";
        echo     "      `basename $0`";
         exit $E_OPTERROR
}


if [ "$1" == "-v" ] ;then
Version
exit 0
fi

if [ "$1" == "-h" ] ;then
Help
exit 0
fi

[ -f $def_conf ] || \
{
echo "Wanning: /usr/local/squid/etc/upload.conf does not exist!";
echo " Programe aborted! ";
exit 1
}


#set the squid.conf entry logfile_rotate directive to 1 or 0, you decide!   
#default resource log is access.log, other logs will be the same format.    
#I strongly recommend you to use the default one. but you decide it.
rsc=log

#you can change it to access.log.0 other logs will change too.
#when there is no log.0, you have to create it by hand, when you first start this 
#program.
#rsc=log.0

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

ifLogBaseDir=$(cat $def_conf|cut -d '#' -f1|awk -F '=' '$1 ~ /LogBaseDir/ {print $2}'| \
        awk '{print $1}'|tail -n1)

ifDevID=$(cat $def_conf|cut -d '#' -f1|awk -F '=' '$1 ~ /DevID/ {print $2}'| \
        awk '{print $1}'|tail -n1 )

ifUpLoad=$(cat $def_conf|cut -d '#' -f1|awk -F '=' '$1 ~ /UpLoad/ {print $2}'| \
	awk '{print $1}'|tail -n1 )

UpLoad=$(echo "$ifUpLoad"|grep -i 'yes')
LogBack=/var/log/chinacache
mkdir -p $LogBack
LogBaseDir=${ifLogBaseDir:-$defLogBaseDir}
Ldir=${LogBaseDir}/archive
Backdir=${LogBaseDir}/backup
#added by xiaosi for multisquid
total_squids=`[ -f /usr/local/squid/etc/flexicache.conf ] && sed -e 's/#.*//g' /usr/local/squid/etc/flexicache.conf|grep cache_processes|awk '{print $2}'`
[ -z $total_squids ] && total_squids=0
#-----------------------------------------------------

DevID=${ifDevID:-$defDevID}
#must not with dir/ ---/
cd $LogBaseDir
ACCESS=${LogBaseDir}/access.${rsc}
CACHE=${LogBaseDir}/cache.${rsc}
STORE=${LogBaseDir}/store.${rsc}


LastModifiedAccess=$(date "+%m%d%H%M%S")
LastModifiedCache=$LastModifiedAccess
LastModifiedStore=$LastModifiedAccess

DstAccess=${DevID}_${LastModifiedAccess}.log
DstCache=${DevID}_cache_${LastModifiedCache}.log
DstStore=${DevID}_store_${LastModifiedCache}.log

#-----change by JYP: modified the log file name, then rotate and last mv to other dir
if [ $total_squids -eq 0 ] ; then
	if test -f $ACCESS
	then
		mv $ACCESS $ACCESS.tmp
	fi

	if test -f $CACHE
	then
		mv $CACHE $CACHE.tmp
	fi

	if test -f $STORE
	then
		mv $STORE $STORE.tmp
	fi

	/usr/local/squid/sbin/squid -k rotate
else
	rm -f $ACCESS.tmp
	rm -f $CACHE.tmp
	rm -f $STORE.tmp

	if [ -f $ACCESS ] ; then
		mv $ACCESS $ACCESS.tmp2
	fi

	for ((i=1;i<=$total_squids; i++)) ; do

#		if [ -f $ACCESS.$i ] ; then
#			mv $ACCESS.$i $ACCESS.tmp2.$i
#		fi

		if [ -f $CACHE.$i ] ; then
			mv $CACHE.$i $CACHE.tmp2.$i
		fi
		
		if [ -f $STORE.$i ] ; then
			mv $STORE.$i $STORE.tmp2.$i
		fi

		/usr/local/squid/sbin/squid -k rotate -A $total_squids -a $i
	done
	sleep 10
	cat $ACCESS.tmp2 >> $ACCESS.tmp
	rm -f $ACCESS.tmp2
	sleep 1
	for ((i=1;i<=$total_squids; i++)) ; do
#	cat $ACCESS.tmp2.$i >> $ACCESS.tmp
#	rm -f $ACCESS.tmp2.$i
#	sleep 1
	cat $CACHE.tmp2.$i >> $CACHE.tmp
	rm -f $CACHE.tmp2.$i
	sleep 1
	cat $STORE.tmp2.$i >> $STORE.tmp
	rm -f $STORE.tmp2.$i
	sleep 1
	done
fi



#added by yuanpeng.jiang for remember access file lists
test -d ${LogBaseDir}/note || mkdir -p ${LogBaseDir}/note
Day=`date +%Y%m%d`

if test -s $ACCESS.tmp
then
mv $ACCESS.tmp ${Ldir}/${DstAccess}
[ "$?" -eq "0" ] && { echo "${DstAccess}" >> ${LogBaseDir}/note/$Day.access.note; } 
fi

if test -s $CACHE.tmp
then
mv $CACHE.tmp ${LogBack}/${DstCache}
#rm -f ${DIR}/archive/${DstCache}
fi

if test -s $STORE.tmp
then
mv $STORE.tmp ${LogBack}/${DstStore}
#rm -f ${DIR}/archive/${DstStore}
fi

