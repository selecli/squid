#!/bin/bash
#
#Script: purgebdb.sh
#
#Function: purge large bdb file
#
#Version: 3.0
#
#Date: 2009-02-16
#
#Author: Yang Nan
#fixbug by liwei ,2010.6.25
#fixbug by dong.zhong ,2010.9.17
#modified by zh for refreshd V3.0, 2011/9/6
#
#BdbFile="/data/refresh_db/rf_squid.pid.db"
BdbPath="/data/refresh_db/"
PurgeBdbLog="/var/log/chinacache/purgebdb.log"
CheckDbLog="/var/log/chinacache/check_db.log"
UpperSize=1073741824	#1G
#CheckSize=500000000
CheckSize=300000000

#CurrentBdbSize=`stat -c %s $BdbFile`
CurrentLogSize=0
RoateSize=5242880
################################################################################
function RotateLog()
{
	[ -e "$PurgeBdbLog" ]&&CurrentLogSize=`stat -c %s $PurgeBdbLog`
	[ "$CurrentLogSize" -ge "$RoateSize" ] || return 1

	for ((N=5;N>=2;N--))
	do
		let M=N-1
		/bin/mv $PurgeBdbLog.$M $PurgeBdbLog.$N 1>/dev/null 2>&1
	done

	/bin/mv $PurgeBdbLog $PurgeBdbLog.1

	[ -e "$CheckDbLog" ]&&CurrentLogSize=`stat -c %s $CheckDbLog`
        [ "$CurrentLogSize" -ge "$RoateSize" ] || return 1

        for ((N=5;N>=2;N--))
        do
                let M=N-1
                /bin/mv $CheckDbLog.$M $CheckDbLog.$N 1>/dev/null 2>&1
        done

        /bin/mv $CheckDbLog $CheckDbLog.1
 }

function dolog()
{
	echo $1 >> $PurgeBdbLog
}

################################################################################
RotateLog

#[ ! -e "$BdbFile" ]&&exit 1

/sbin/pidof refreshd > /dev/null || {
	dolog "`/bin/date +"%Y-%m-%d %H:%M:%S"` refreshd is not running."
	exit 0
}

/bin/netstat -an|grep 21108 |grep ESTABLISHED > /dev/null || {
	dolog "`/bin/date +"%Y-%m-%d %H:%M:%S"` refreshd connection do not ESTABLISHED"
	exit 0
}

for BdbFile in $(ls ${BdbPath}*.db)
do
	CurrentBdbSize=`stat -c %s $BdbFile`
#	echo $BdbFile
#	echo $CurrentBdbSize
	if [ "$CurrentBdbSize" -ge "$CheckSize" ]
	then
		/usr/local/squid/bin/check_db $BdbFile > /dev/null 2>&1
                dolog "`/bin/date +"%Y-%m-%d %H:%M:%S"` size of $BdbFile is $CurrentBdbSize bytes,uniq  bdb file successful!"
	fi

	CurrentBdbSize=`stat -c %s $BdbFile`

	if [ "$CurrentBdbSize" -ge "$UpperSize" ]
	then
		echo >$BdbFile
		/usr/local/squid/bin/refreshd -k url_verify
		dolog "`/bin/date +"%Y-%m-%d %H:%M:%S"` size of $BdbFile is $CurrentBdbSize bytes,purge bdb file successful!"
		exit 0
	else
		dolog "`/bin/date +"%Y-%m-%d %H:%M:%S"` size of $BdbFile is $CurrentBdbSize bytes,check bdb file successful!"
	fi
done
