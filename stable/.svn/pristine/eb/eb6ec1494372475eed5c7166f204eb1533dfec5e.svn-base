#!/bin/bash

#Author: weiguo.chen@chinacache.com
# Function:
#		synchronization FC time
# History:

#########################functions############################

#brief:
#   log message to logfile
#params:
#   $1 message
#return:
#	NULL
logMsg()
{
	echo "$1" >> $Logfile
}

#brief:
#	functions to user
#params:
#   $0  shell name
#return:
#	NULL
Help( )
{
        echo Example:
        echo     "      `basename $0`    "; 
        echo     "      `basename $0` -h ";
        echo     "      `basename $0` -v ";
        exit $E_OPTERROR 
}

#brief:
#	version information
#params:
#	$0 shell name
#return:
#	NULL
Version( )
{
        echo     "      `basename $0` 1.0.6 
                        Last modified at 2014-6-6
                        Written by cwg 
                        EmailTo:   weiguo.chen@chinacache.com
                        Copyright (C) ChinaCache Co.,Ltd."
}

#brief:
#	Synchronization FC time
#params:
#	$1		the ip of ntpdated
#return:
#	0		fail
NtpSyn( )
{
	[ "$1" = '' ]&&return 0
    LOG=`/usr/sbin/ntpdate $1 2>&1`
	HasOffset=`echo $LOG|grep -i offset`
	if test -n "$HasOffset";then
		logMsg $LOG
        exit 0
	fi	
}

#brief:
#	last time Synchronization FC time
#params:
#	$1		the ip of ntpdated
#return:
#	NULL
NtpSynErr( )
{
	LOG=`/usr/sbin/ntpdate $1 2>&1`
	logMsg $LOG
}

#brief:
#	Synchronization FC time, use domain and dns
#params:
#	NULL
#return:
#   NULL
reslove_domain()
{
	while read server
	do  
		ip=$(dig -t A $domain @$server +noall +answer | perl -ane 'if($F[3] eq "A"){print "$F[-1]\n";}')
		while read ip_t 
		do
			NtpSyn $ip_t 
			done< <(echo $ip| awk '{for(n=1;n<=NF;n++){print $n}}')
	done < <(awk '/^#/{next}/^external_dns/{split($NF,a,"|" );for(i in a)print a[i];}' $ConfFile)
}

#brief:
#	Synchronization FC time, use confirm ip
#params:
#	NULL
#return:
#   NULL
reslove_ip()
{
	i=0
	while read Entry Value Others
	do
		if [ "$Entry" = "NtpSyn" ];then
			NtpSynVal[${i:-0}]=$Value
			i=$((${i:-0} + 1))
		elif [ "$Entry" = "NtpSynErr" ];then
		    NtpSynErrVal=$Value
		fi
	done < <(test -r $ConfFile&&grep -v '^#' $ConfFile|grep -v '^$')

	if [ "$i" -ge 1 ];then
		for ((x=0;x<$i;x++));do
			NtpSyn ${NtpSynVal[$x]}
		done
	else
		NtpSyn 192.36.143.150  
		NtpSyn 193.170.141.4
		NtpSyn 61.19.242.42
	fi
}

#brief:
#	Synchronization FC time, use do not confirm ip
#params:
#	NULL
#return:
#   NULL
reslove_default ()
{
	i=0
	while read Entry Value Others
	do
		if [ "$Entry" = "NtpSynErr" ];then
		    NtpSynErrVal=$Value
		fi
	done < <(test -r $ConfFile&&grep -v '^#' $ConfFile|grep -v '^$')

	if [ -n "$NtpSynErrVal" ];then
		NtpSynErr $NtpSynErrVal
	else
		NtpSynErr 61.206.115.3
	fi
}
#################################main###########################

source /usr/local/squid/bin/functions 
tryLockProcess $0
if [ $? -ne 0 ] ;then exit 0;fi     
export PATH=.:/usr/local/squid/sbin:/usr/local/squid/bin:/usr/kerberos/sbin:/usr/kerberos/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin:/root/bin
Logfile=/var/log/chinacache/ntpsync.log
ConfFile=/usr/local/squid/etc/fcexternal.conf
domain=ntp.chinacache.com

if [ "$1" == "-v" ] ;then
    Version
    exit 0
fi

if [ "$1" == "-h" ] ;then
    Help
    exit 0
fi

#### try domain   ####
reslove_domain

#### try right ip ####
reslove_ip

#### try donot confirm ip ####
reslove_default


UTC=`cat /etc/sysconfig/clock | grep UTC | awk -F '=' '{print $2}'`
if [ $UTC = "true" ];then
	    hwclock --systohc --utc
fi
if [ $UTC = "false" ];then
		hwclock --systohc --localtime
fi
unlockProcess $0
exit 0
