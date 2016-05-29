#!/bin/sh
i=0
j=0
while true;
do
	cmd="curl -s -v -o /dev/null -H HOST:www.china.com.cn http://127.0.0.1/ad/yigao.htm?$j &"
	echo $cmd

	eval $cmd

	((j++))
	if [ $j -eq 10000 ]; then
		((i++))
		j=0
	fi
done
