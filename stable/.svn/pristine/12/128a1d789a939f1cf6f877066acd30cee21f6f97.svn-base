#!/bin/bash

#This is used for async_hit only in OFFLINE MOD
#Written By yuanpeng.jiang@chinacache.com

LOG_PATH=/data/proclog/log/squid/async.log
LOG_PATH_TMP=/data/proclog/log/squid/async.$$


#if [ -f $LOG_PATH ]
#then
#	echo "$LOG_PATH exists!"
#else
#	echo "$LOG_PATH not exists!"
#	exit 0
#fi

WGET_NU=`ps -ef | grep curl| wc -l`
WGET_GAP=50

if [ "$WGET_NU" -gt "$WGET_GAP" ]
then
	exit 0
fi


CUR_TIME=`date +%s`
CUR_TIME_GAP=$((CUR_TIME-60))
total_squids=`[ /usr/local/squid/etc/flexicache.conf ] && sed -e 's/#.*//g' /usr/local/squid/etc/flexicache.conf|grep cc_multisquid|awk '{print $2}'`
[ -z $total_squids ] && total_squids=0
init_port=`sed -e 's/#.*//g' /usr/local/squid/etc/squid.conf|grep http_port|awk '{print $2}'`

if [ $total_squids -eq 0 ] ; then
	awk '$1 > '$CUR_TIME_GAP'{print $2" "$3}' $LOG_PATH > $LOG_PATH_TMP

	echo -n "" > $LOG_PATH

	cat $LOG_PATH_TMP |sort|uniq|\
	while read URL compress
	do
		HOST=`echo $URL | awk -F'/' '{print $3}'`
		FULL=`echo $URL | sed 's/http\:\/\/'${HOST}'\//http\:\/\/127\.0\.0\.1\//g'`
		if [ "x$compress" = "xc" ]
		then    
			curl -s -o /dev/null $FULL -H Host:$HOST --compress &
		else    
			curl -s -o /dev/null $FULL -H Host:$HOST &
		fi      
	done

	rm -rf $LOG_PATH_TMP

else

	for ((i=1;i<=$total_squids; i++)) ; do
		awk '$1 > '$CUR_TIME_GAP'{print $2" "$3}' ${LOG_PATH}.${i} > $LOG_PATH_TMP
	
		echo -n "" > ${LOG_PATH}.${i}
	
		cat $LOG_PATH_TMP |sort|uniq|\
		while read URL compress
		do
			HOST=`echo $URL | awk -F'/' '{print $3}'`
			FULL=`echo $URL | sed 's/http\:\/\/'${HOST}'\//http\:\/\/127\.0\.0\.1':$(($init_port+$i))'\//g'`
			if [ "x$compress" = "xc" ]
			then
				curl -s -o /dev/null $FULL -H Host:$HOST --compress &
			else
				curl -s -o /dev/null $FULL -H Host:$HOST &
			fi
		done
	
		rm -rf $LOG_PATH_TMP
	done
fi
exit 0
