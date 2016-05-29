#!/bin/bash

#******************************
# FC Check
# peng.zhang@chinacache.com
#******************************

cache_log="/data/proclog/log/squid/cache.log"
cache_log_bak=/var/log/chinacache/fc-cache.log.`cat /sn.txt`.`hostname`.`date +"%Y%m%d"`*.gz
FCBug="FATAL:|assertion failed:|Killing RunCache"

CheckSquidRunTime()
{
	if [ $(ps -ef | grep "(squid)"|grep -v grep|wc -l) -ge 1 ]; then
		restartUTCTime=`/usr/local/squid/bin/squidclient -h localhost -p 80 mgr:info|grep -E "Start Time"|awk -F "Time:" '{print $2}'|sed 's/^\s//'`
		restartTime=`date -d "$restartUTCTime" +"%Y/%m/%d %H:%M:"`
		if [ `echo $(($(date +"%s") - $(date -d "$restartUTCTime" +"%s")))` -lt 86400 ];then 
			echo "CheckSquidRunTime:	FC RunTime < 24 hours!"
			CheckCacheLog
		else
			echo "CheckSquidRunTime:	FC RunTime > 24h"
		fi
	else
		echo "CheckSquidRunTime:	Please start squid !"
		CheckCacheDir
		CheckCacheLog
		exit 0
	fi
	echo "-  -  -  -  -  -  -  -  -  -  -  -  -  -  -"
}

CheckCacheLog()
{
	if [ -f $cache_log_bak ]; then
		zcat $cache_log_bak| grep -P -A 1 "$FCBug"
	fi
		tac $cache_log|grep -P -A 1 "$FCBug"
}

CheckFCObjects()
{
	if [ `/usr/local/squid/bin/squidclient -h localhost -p 80 mgr:info|grep on-disk |awk '{print $1}'` -gt 20000000 ];then
		echo "CheckFCObject:	Objects > 2000W !"
		echo "-  -  -  -  -  -  -  -  -  -  -  -  -  -  -"
	else
		echo "CheckFCObject:	OK"
	fi
}

CheckFCMemory()
{
	obj_num=`/usr/local/squid/bin/squidclient -h localhost -p 80 mgr:info|grep StoreEntries|head -n 1|awk '{print $1}'`
	mem=`/bin/ps h -orss -p $(/sbin/pidof '(squid)')`
	coss_mem=`/usr/local/squid/bin/squidclient -h localhost -p 80 mgr:coss|grep -P ^stripes:|awk '{print $2*1024}'`
	if [[ `echo $(($mem - $coss_mem))` -gt 2500000 ]] && [[ $obj_num -lt "14000000" ]]; then
		echo "CheckFCMemory:	Memory Exception !"
		echo "squid_mem=$mem"
		echo "objects_num=$obj_num"
		echo "-  -  -  -  -  -  -  -  -  -  -  -  -  -  -"
	else
		echo "CheckFCMemory:	OK"
	fi
}

CheckRefreshdRunTime()
{
	if [ $(ps -ef | grep "(squid)" | grep -v grep | wc -l) -ge 1 ]; then 
		if [ $(ps -ef | grep refreshd | grep -v grep | wc -l) -ge 1 ]; then
			restartTime=$(ps -o start_time -p `pidof refreshd`|grep -v START) 
            if [ `echo $(($(date +"%s") - $(date -d  "$restartTime" +"%s")))` -lt 86400 ]; then
			 	echo "CheckRefreshdRunTime:	Refreshd RunTime < 24 hours !"
			 	echo "RestartTime=$restartTime"
			else
				echo "CheckRefreshdRunTime:	OK"
			fi
		else 
			echo "CheckRefreshdRunTime:	Squid is already running, but refreshd did not start !"
		fi
	else
		echo "CheckRefreshdRunTime:	Please start squid !"
	fi
	echo "-  -  -  -  -  -  -  -  -  -  -  -  -  -  -"
}

CheckBillingdRunTime()
{
	if [ $(ps -ef | grep "(squid)"|grep -v grep|wc -l) -ge 1 ]; then  
		if [ $(ps -ef | grep billingd|grep -v grep|wc -l) -ge 1 ]; then 
			restartTime=$(ps -o start_time -p `pidof billingd`|grep -v START)
			if [ `echo $(($(date +"%s") - $(date -d  "$restartTime" +"%s")))` -lt 86400 ]; then
				echo "CheckBillingdRunTime:	billingd RunTime < 24 hours !"
			 	echo "RestartTime=$restartTime"
			else
				echo "CheckBillingdRunTime:	OK"
			fi
		else 
			echo "CheckBillingdRunTime:	Squid is already running, but billingd did not start !" 
		fi
	else
		echo "CheckBillingdRunTime:	Please start squid !"
	fi
	echo "-  -  -  -  -  -  -  -  -  -  -  -  -  -  -"
}

CheckBillingdLog()
{
	[[ `ps aux | grep '(squid'| grep -v grep | wc -l` -gt 0 ]]&&[[ `cat /usr/local/squid/etc/squid.conf | grep "mod_billing on" | grep -v '#' |wc -l` -gt 0 ]] &&[[ `ps aux | grep billingd | grep -v grep | wc -l` -eq 0 ]] && echo "CheckBillingd:	error" || echo "CheckBillingd:	OK"
}

CheckBillingLogOut()
{
	if [ `ps aux | grep '(squid'| grep -v grep | wc -l` -eq 1 ]; then
		DATE=`grep entry_count /var/log/chinacache/billingd_info.log | tail -n 1 | awk -F ']' '{print $1}' |awk -F '[' '{print $2}'`
		time_last=`date +%s -d "$DATE"`
		time_now=`date +%s`
		past=$(($time_now-$time_last))
		if [ $past -gt 1800 ]; then
			echo "CheckBillingLogOut:	Billingd output billinglog > 1800s"
		else
			echo "CheckBillingLogOut:	OK"
		fi
	else
		echo "CheckBillingLogOut:	Please start squid !"
	fi
	echo "-  -  -  -  -  -  -  -  -  -  -  -  -  -  -"
}

CheckOOM()
{
	if cat /var/log/messages | grep -Pi "Out of memory" > /dev/null; then
		OOM=`cat /var/log/messages | grep -Pi "Out of memory"`
		echo "CheckOM:	$OOM"
		echo "-  -  -  -  -  -  -  -  -  -  -  -  -  -  -"
	else 
		echo "CheckOOM:	OK"
	fi
}

CheckCacheDir()
{
	content=$(zcat $cache_log_bak| grep -A1 "Permission denied")
	content2=$(tac $cache_log| grep -A1 "Permission denied")
	num=$(zcat $cache_log_bak| grep "Permission denied" |wc -l)
	num2=$(tac $cache_log| grep "Permission denied" |wc -l)
	if [[ $num -gt 0 || $num2 -gt 0 ]]; then 
		echo "CheckCacheDir:"
		if [ -f $cache_log_bak ]; then
			echo $content 
		fi
		echo $content2
	else
		echo "CheckCacheDir:	OK"
	fi
	echo "-  -  -  -  -  -  -  -  -  -  -  -  -  -  -"
}

echo "Check ... "
echo "*  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *"
CheckSquidRunTime
CheckFCObjects
CheckFCMemory
CheckRefreshdRunTime
CheckBillingdRunTime
CheckOOM
CheckBillingLogOut
echo "*  *  *  *  *  *  *  *  *  *  *  *  *   *  *  *"
echo "Check end ... "
