#!/bin/sh

echo "------------------------------------>cpu  mem% process"
while true;do
	now=$(date)
	pid=$(cat /var/run/refreshd.pid)

	s=$(top -p $pid -n 1|grep refreshd|awk '{print $9,$10,$12}')

	if [[ $l_s != $s ]] ;then
		echo -n "$now ------->"
		echo "$s"
	fi

	l_s=$s
		
	sleep 2
done | tee -a /tmp/mon_refreshd.log
