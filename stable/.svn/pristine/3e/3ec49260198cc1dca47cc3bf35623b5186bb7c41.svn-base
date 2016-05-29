#!/bin/bash

#The script will be used for FC6 to merge more than one cache.log into one!
source /usr/local/squid/bin/functions 
tryLockProcess $0
if [ $? -ne 0 ] ;then exit 0;fi     

export PATH=.:/usr/local/squid/sbin:/usr/local/squid/bin:/usr/kerberos/sbin:/usr/kerberos/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin:/root/bin

defLogBaseDir=/data/proclog/log/squid
rsc=log
CACHE=${defLogBaseDir}/cache.${rsc}
separator=-------------------------------------------------------------------------------------------

#added by robin for multisquid
total_squids=`[ -f /usr/local/squid/etc/flexicache.conf ] && sed -e 's/#.*//g' /usr/local/squid/etc/flexicache.conf|grep cache_processes|awk '{print $2}'`
[ -z $total_squids ] && total_squids=0
#-----------------------------------------------------

if [ $total_squids -eq 0 ] ; then
	unlockProcess $0
	exit
else

	for ((i=1;i<=$total_squids; i++)) ; do

		if [ -f $CACHE.$i ] ; then
			cat $CACHE.$i >>$CACHE
			echo $separator >>$CACHE
			cat /dev/null >$CACHE.$i
		fi
		
	done
fi
unlockProcess $0
