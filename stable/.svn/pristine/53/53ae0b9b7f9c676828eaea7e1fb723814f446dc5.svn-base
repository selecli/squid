#!/bin/bash

# Chinacache SquidDevTeam
# $Id$
export PATH=.:/usr/local/squid/sbin:/usr/local/squid/bin:/usr/kerberos/sbin:/usr/kerberos/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin:/root/bin
Logfile=/var/log/chinacache/getIptables.log
ConfFile=/usr/local/squid/etc/fcexternal.conf
DefaultIptablesAddr="http://61.135.207.18/squid/iptables"

Help( )
{
        echo Example:
        echo     "      `basename $0`    "; 
        echo     "      `basename $0` -h ";
        echo     "      `basename $0` -v ";
         exit $E_OPTERROR 
}
Version( )
{
        echo     "      `basename $0` 1.0.1 
                        Last modified at 2007-07-30                       
                        Written by Alex Yu
                        EmailTo:   alex.yu@chinacache.com
                        Copyright (C) ChinaCache Co.,Ltd."
}

getIptables()
{
	local IptablesAddr=$1
	wget -T ${IptablesTimeout:-20} -t3 $1 -O /etc/sysconfig/iptables
	if [ -r /etc/sysconfig/iptables ];then
		diff /etc/sysconfig/iptables /etc/sysconfig/iptables.bak 1>/dev/null 2>&1
		if [ "$?" -eq 0 ];then
			:
		else
			/sbin/iptables-restore /etc/sysconfig/iptables
			if [ "$?" -eq 0 ];then
				mv -f /etc/sysconfig/iptables /etc/sysconfig/iptables.bak
				echo "iptables-restore complete at `date`" >>$Logfile
				exit 0
			else
				echo "iptables-restore failed at `date`" >>$Logfile
			fi	
		fi
	fi	
	
	
}

if [ "$1" == "-v" ] ;then
Version
exit 0
fi

if [ "$1" == "-h" ] ;then
Help
exit 0
fi

#parse configuration
i=0
while read Entry Value Others
do
	if [ "$Entry" = "IptablesAddr" ];then
		IptablesAddr[${i:-0}]=$Value
		i=$((${i:-0} + 1))
	fi
        if [ "$Entry" = "IptablesTimeout" ];then
                IptablesTimeout=$Value
        fi
	
done < <(test -r $ConfFile&&grep -v '^#' $ConfFile|grep -v '^$')

#processing
if [ "$i" -ge 1 ];then
	for ((x=0;x<$i;x++));do
		 getIptables ${IptablesAddr[$x]}
	done
else
#default processes
	getIptables  $DefaultIptablesAddr
fi

exit 0
