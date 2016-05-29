#!/bin/sh

#******************************************
#Desc:   check squid log upload status 
#Author: qiang.guo 
#Date :  2007-11-08
#*****************************************

WarningNumber=10
MON_POOL_DIR='../pool'
log_dir=$(set -- $(grep "log_dir" "../etc/monitor.conf");echo $2)
lognum_cache=`ls $log_dir/*_*_*.log 2>/dev/null|grep -v cache |wc -l`
lognum_billing=`ls $log_dir/*billing* 2>/dev/null |wc -l`

[ "$lognum_cache" -ge "$WarningNumber" ]&&echo "$lognum_cache"&&exit 1
[ "$lognum_billing" -ge "$WarningNumber" ]&&echo "$lognum_billing"&&exit 1
echo 0
