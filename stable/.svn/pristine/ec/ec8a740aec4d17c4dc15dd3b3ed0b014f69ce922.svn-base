#!/bin/sh
i=45
j=0
while true;
do
	cmd="wget http://www.linux-ha.org/$i/$j >/dev/null 2>&1"

	eval $cmd

	((j++))
	if [ $j -eq 10000 ]; then
		echo $cmd
		((i++))
		j=0
	fi

	if [ $i -eq 1000 ] ; then
		break
	fi	
done
