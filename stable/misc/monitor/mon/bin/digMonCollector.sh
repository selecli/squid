#!/bin/bash

# Chinacache SquidDevTeam
# $Id$

#It monitors detectorig's dig Status.
#wild. error 192.168.1.23:192.168.1.243:192.168.1.222:202.106.0.10
DEBUG=0
INTERVAL=5
DataFile=/usr/local/squid/var/dig_alert.dat
test -r $DataFile|| { echo '0';echo 'Fatal:';echo "Error:";exit 1; }

#--------------------------------------------------------------------
if false;then
M_DATE=`stat $DataFile|awk '/Modify/ {print $2}'`
M_TIME=`stat $DataFile|awk '/Modify/ {print $3}'|cut -d. -f1`


L_ym=${M_DATE%-*}
#echo \$L_ym=$L_ym
L_y=${M_DATE%%-*}
#echo \$L_y=$L_y
L_m=${L_ym#*-}
#echo \$L_m=$L_m
L_d=${M_DATE##*-}
#echo \$L_d=$L_d

#echo $M_TIME
L_HM=${M_TIME%:*}
#echo \$L_HM=$L_HM
L_H=${M_TIME%%:*}
#echo \$L_H=$L_H
L_M=${L_HM#*:}
#echo \$L_M=$L_M
L_S=${M_TIME##*:}
#echo \$L_S=$L_S
INVALID=`awk -v _tm_test_date="$L_y $L_m $L_d $L_H $L_M $L_S" \
-v interval="$INTERVAL" 'BEGIN  {
    if (_tm_test_date) {
        t = mktime(_tm_test_date)
        now = systime()
        diff = (now - t)/60
        printf "%d\n", (diff - interval)
    }
}' `

#echo $INVALID
if [ "$INVALID" -ge "0" ];then
        SNMPVAL=-1
        echo $SNMPVAL
        echo '-1'
        echo '-1'
        exit 1
fi
#comment out
fi
#----------------------------------------------------------------------------------

while read Host Status Ips Others
do
        if [ "$Status" = "fatal" ];then
                Fatal[${i:-0}]="$Host"
                i=$((${i:-0} + 1))
        elif [ "$Status" = "error" ];then
                Error[${j:-0}]="$Host"
                j=$((${j:-0} + 1))
#        elif [ "$Status" = "warning" ];then
#                Warning[${j:-0}]="$Host"
#                k=$((${k:-0} + 1))
        fi

done< $DataFile

#_______________________________


if [ "${i:-0}" -ge 1 ];then
	WarnVal100=$i
        for ((x=0;x<$i;x++));do
		FatalHost="${FatalHost:-"Fatal:"} ${Fatal[$x]}"
        done
fi

if [ "${j:-0}" -ge 1 ];then
	WarnVal=$j
        for ((x=0;x<$j;x++));do
                ErrorHost="${ErrorHost:-"Error:"} ${Error[$x]}"
        done
fi

Result=$(( ${WarnVal100:-0} * 100 + ${WarnVal:-0} ))

[ $DEBUG -ge 1 ]&&echo "Outputs-------------------------------------------------------"
echo "$Result"
echo "${FatalHost:-Fatal:}"  
echo "${ErrorHost:-Error:}"
