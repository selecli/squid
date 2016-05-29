#!/bin/bash
while :
do
		#cat /monitor/pool/{squidResponse,netConn,netBandwidth,load}Mon.pool
        cat /monitor/pool/squidResponseMon.pool
        for((i=0;i<60;i++));do
                echo -n .
                sleep 1
        done
        echo 
done
