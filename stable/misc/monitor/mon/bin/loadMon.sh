#!/bin/sh

#******************************************
#Desc:   check system load
#Author: Qingrong.Jiang
#Date :  2006-03-06
#*****************************************

MON_POOL_DIR='../pool'

loadAve1=`cat /proc/loadavg | awk '{print $1}'`
loadAve5=`cat /proc/loadavg | awk '{print $2}'`
loadAve15=`cat /proc/loadavg | awk '{print $3}'`
echo loadAve1: $loadAve1 > ${MON_POOL_DIR}/loadMon.pool
echo loadAve5: $loadAve5 >> ${MON_POOL_DIR}/loadMon.pool
echo loadAve15: $loadAve5 >> ${MON_POOL_DIR}/loadMon.pool
