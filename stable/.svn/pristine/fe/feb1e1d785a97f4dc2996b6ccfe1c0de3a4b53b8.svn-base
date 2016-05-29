#!/bin/bash
# API for NG
# Author: GSP/chenqi
# Date: 25 Apr. 2013
# Note: This is a draft copy, to be imporved...

Usage() {
	echo "Usage: $0 [-cmtrfh]"
		echo " -c   Got CPU Usage."
		echo " -m   Got Memory Usage."
		echo " -t   Got Response Time."
		echo " -r   Got Requests per Second."
		echo " -f   Got Refresh Client State." 
		echo " -h   Print Help Message."
}

TellMode() {
	local mode=$(echo `/sbin/service squid status` | /bin/awk '{print $NF}')
	local stat="single"
	local cmdline="none"
	[ -f /var/run/flexicache.pid -a -f /var/run/nginx.pid -a -f /var/run/squid.pid.1 ] && stat="multi"

	if [ "$stat" == "multi" ];then
		local pidFile=/proc/`cat /var/run/flexicache.pid`/cmdline
		if [ ! -f "$pidFile" ];then
			stat="single"
		else
			cmdline=$(cat $pidFile)
			if [ "$cmdline" != "/usr/local/squid/bin/flexicache" ];then
				stat="single"
			fi
		fi
	fi

	if [ "$mode" = "running..." -a "$stat" = "multi" ];then
		return 2
	elif [ "$mode" = "running..." -a "$stat" = "single" ];then
		return 1
	else
		return 0
	fi
}

TellNumOfSquid() {
	local Version=$(/usr/local/squid/sbin/squid -v | head -1 | awk '{print $NF}')
	local v=${Version%%\.*}
	if [ "$v" = "5" ]; then		# FC5.x
		return 1
	elif [ "$v" = "6" ]; then     # FC6.x
		total_squids=$(sed -e 's/#.*//g' /usr/local/flexicache/etc/flexicache.conf|grep cache_processes|awk '{print $2}')
		return $total_squids
	elif [ "$v" = "7" ]; then     # FC7.x
		TellMode
		local result=$?
		if [ $result -eq 2 ]; then
			total_squids=$(sed -e 's/#.*//g' /usr/local/squid/etc/flexicache.conf|grep cache_processes|awk '{print $2}')
			return $total_squids
		else
			return 1
		fi
	fi
}


GetProcessCpu() {
	if [ ! -f $1 ];then
		return 0
	fi
	local pid=$(cat $1)
	if [ ! -f "/proc/$pid/cmdline" ];then	# process does not exist
		return 0
	fi

	local add=$(top -b -d 1 -n 1 -p $pid | grep $pid | awk '{print $9}')
	ret_cpu=$(: | awk -v a="${add}" -v b="${ret_cpu}" 'END{c=a+b;print c}')
}

GetCPUInfo() {
	ret_cpu=0
	if [ $1 -eq 1 ]; then 
		GetProcessCpu "/var/run/squid.pid"
	else
		GetProcessCpu "/var/run/nginx.pid"
		for ((i=1;i<=$1;i=i+1)); do
			GetProcessCpu "/var/run/squid.pid.$i"
		done
	fi
	echo $ret_cpu
}

GetProcessMem() {
	if [ ! -f $1 ];then
		return 0
	fi
	local pid=$(cat $1)
	if [ ! -f "/proc/$pid/statm" ];then
		return 0
	fi
	local page=$(cat "/proc/$pid/statm" | awk '{print $1}')
	ret_mem=$((ret_mem+page*4))		# page, each page got 4 KB
}

GetMemInfo() {
	ret_mem=0
	if [ $1 -eq 1 ]; then 
		GetProcessMem "/var/run/squid.pid"
	else
		GetProcessMem "/var/run/nginx.pid"
		for ((i=1;i<=$1;i=i+1)); do
			GetProcessMem "/var/run/squid.pid.$i"
		done
	fi
	echo $((ret_mem/1024))		# MB
}


GetResponseTime() {
	cd /monitor/bin
	awk 'BEGIN{rt=-1;time=0;} {if(/^squidResponseTime/){rt=$2;}else if(/^squidResponseUpdateTime/){time=$2;} }END{if(systime()-time >=68){print "-1";}else{print rt;}}' /monitor/pool/squidResponseMon.pool 2>/dev/null|| echo "-1"
}


GetProcessRPS() {
	local add=$(/usr/local/squid/bin/squidclient -p $1 mgr:5min | grep "client_http.requests" | awk '{print $3}')
	add=${add%%\/sec}
	request_per_second=$(: | awk -v a="${add}" -v b="${request_per_second}" 'END{c=a+b;print c}')
}

GetRequestsPerSec() {
	request_per_second=0
	if [ $1 -eq 1 ]; then 
		GetProcessRPS 80
	else
		for ((i=1;i<=$1;i=i+1)); do
			port=$((i+80))
			GetProcessRPS $port
		done
	fi
	echo $request_per_second 
}


GEtRefreshInfo() {
	refresh_cli -f http://127.0.0.1/ 2>&1 |awk 'BEGIN{ret=2;} /url_ret.*200/ {ret = 1;} END{print ret;}'
}

#-------------------Script Running Here----------------------------------------

if [ $# -ne 1 ];then
	Usage
	exit 1   
fi

while getopts "cmtrfh" options;do
case $options in
	c)  option=0;;
	m)  option=1;;
	t)  option=2;;
	r)  option=3;;
	f)  option=4;; 
	h)  Usage; exit 0;;
esac
done

TellNumOfSquid
result=$?
case $option in
	0)	GetCPUInfo $result ;;		# CPU
	1)	GetMemInfo $result ;;		# Memory
	2)	GetResponseTime $result ;;		# response time
	3)	GetRequestsPerSec $result ;;	# requests/s
	4)	GEtRefreshInfo $result ;;	# refresh_cli
esac

#---------------Script End------------------------------------------------------

