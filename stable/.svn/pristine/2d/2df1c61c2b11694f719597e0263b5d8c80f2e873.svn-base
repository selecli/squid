#!/bin/bash

DEBUG=0
Key=''

calculateSpeed()
{
	local patition=$1
	 [ "$DEBUG" -ge 2 ]&&echo " In function patition=$1" 

	Before=`date +%s.%N`
	  [ "$DEBUG" -ge 2 ]&& echo "Before=$Before"
	for((i=1;i<=10;i++));do
	dd if=/dev/zero of=${1}/diskspeed_${i}.$$  bs=409 count=2500 1>&2 2>/dev/null 
	done
	After=`date +%s.%N`
	  [ "$DEBUG" -ge 2 ]&& echo "After=$After"
	
	 [ "$DEBUG" -ge 2 ]&&echo "thetime=`echo "$After - $Before"|bc`"
	thetime=`echo "$After - $Before"|bc` 
	
	rm -f ${1}/diskspeed_*.$$

	[ "$DEBUG" -ge 2 ]&&echo "Sec =$thetime"

	speed=`echo "scale=6;10.0 / $thetime"|bc`

	 [ "$DEBUG" -ge 2 ]&& echo "\$speed=$speed"
	printf %.2f "$speed"
}


#calculateSpeed  /data/proclog

##-----------
#df -h | awk '$1 ~ "^/dev/"&& $6 != "/boot" && "/" {print $1"    " $6}' >awk.out

rmin=1000000
wmin=1000000
while read n1 n2
do
	[ "$DEBUG" -ge 1 ]&&echo "-----Inputs: $n1 $n2 -------------"
	[[ ${n1: -1} =~ [0-9] ]]&&n1=${n1:5:3} 
	if [[ $Key =~ $n1 ]];then
		:
	else
		SpeedRead=`/sbin/hdparm -t /dev/${n1} 2>/dev/null|awk  '$0 ~ /seconds/ {print $11}'`
		
		SpeedWrite=`calculateSpeed $n2`
		Key="$Key $n1"
		 [ "$DEBUG" -ge 1 ]&&echo "Key=[ $Key ]"
		 [ "$DEBUG" -ge 1 ]&&echo "n1:$n1:$SpeedRead:$SpeedWrite"
		[ `echo "$SpeedRead < $rmin"|bc` -eq 1 ]&&rmin=$SpeedRead&&rdir=$n1
		[ `echo "$SpeedWrite < $wmin"|bc` -eq 1 ]&&wmin=$SpeedWrite&&wdir=$n1
	fi  

done < <(df -h | awk '$1 ~ "^/dev/"&& $6 != "/boot" &&  $6 != "/" {print $1"    " $6}')

[ "$DEBUG" -ge 1 ]&&echo "The slowest read dir is $rdir ,speed is $rmin MB/s"
[ "$DEBUG" -ge 1 ]&&echo "The slowest write dir is $wdir ,speed is $wmin MB/s"

#diskWriteSpeedMBps: 260.08
#diskReadSpeedInMBps: 618.43
#diskSpeedUpdateTime: 1168487676
#
echo -e "diskWriteSpeedMBps: $wmin
diskReadSpeedInMBps: $rmin
diskSpeedUpdateTime: `date +%s`" \
	>  /monitor/pool/diskSpeedMon.pool
[ "$DEBUG" -ge 1 ]&&echo -e "diskWriteSpeedMBps: $wmin
diskReadSpeedInMBps: $rmin
diskSpeedUpdateTime: `date +%s`"
exit 0



