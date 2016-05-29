#!/bin/bash

while read patition;do
	Date=$(date "+%m%d%H%M%S")
	echo "$patition $Date" > ${patition}/diskstatus.$$;
	[ "$patition $Date" = "$(cat ${patition}/diskstatus.$$ 2>&1)" ]||Status=1
	rm -f ${patition}/diskstatus.$$
done < <(df -h|awk '$1 ~ /dev/ {print $6}')
IoWait=$(iostat |sed -n "4 p"|awk '{print $4}')
sleep 1
IoWait2=$(iostat |sed -n "4 p"|awk '{print $4}')
sleep 1
IoWait3=$(iostat |sed -n "4 p"|awk '{print $4}')
#results
AvgIoWait=$( echo "scale=2;($IoWait + $IoWait2 + $IoWait3)/3"|bc )



echo "${Status:-0}"
printf "%.2f\n" $AvgIoWait
