#!/bin/bash

# Chinacache R&D Dept. FlexiCache Team
#---------------------------------------------------------------------------|
#  @program   : purgeGetopts.sh                                             |  
#  @version   : 6.0.1 , only support "-A ALL"                               |
#  @function@ : to purge squid cache objects.                               |
#  @campany   : china cache                                                 |
#  @dep.      : Chinacache R&D Dept. FlexiCache Team                        |
#  @writer    : Alex  Yu                                                    |
#  @modified  : yuanpeng.jiang,liwei,Yangnan,xiaosi                         |
#---------------------------------------------------------------------------|

DEBUG=0
E_OPTERROR=65
LogFile=/var/log/chinacache/purgedir.log

Help( )
{
        echo Example:
        echo     " `basename $0` -A ALL  purge cache";
        echo     " `basename $0` -h : show help message";
        echo     " `basename $0` -v : show version";
        exit $E_OPTERROR
}
Version( )
{
        echo     "      `basename $0` 6.0.2
                        Last modified at 2009-02-13
                        Written by Alex Yu
                        Modified by yuanpeng.jiang,liwei,Yangnan
                        EmailTo:   alex.yu@chinacache.com
                        Copyright (C) ChinaCache Co.,Ltd."
}
purgeCache()
{
        local Dir=$1

        echo "$(date "+%m-%d %H:%M:%S") $Dir is purged">> $LogFile
        [ "$DEBUG" -ge "1" ] &&echo "In purgeDir purgeing $Dir"
        N=1
        Conf=/usr/local/squid/etc/squid.conf
	FCConf=/usr/local/squid/etc/flexicache.conf
	if [ -f $FCConf ] ; then
		total_squids=`sed -e 's/#.*//g' $FCConf|grep cache_processes|awk '{print $2}'`
	else
		total_squids=0
	fi
	[ -z $total_squids ] && total_squids=0
        [ $total_squids -eq 0 ] && pkill -9 squid 2>&1 1>> $LogFile || (pkill -9 flexicache && pkill -9 haproxy && pkill -9 squid)
        pkill -9 refreshd >/dev/null 2>&1
        for x in $(awk '$1 ~ /^cache_dir/ {if ($3 ~ /.*\/$/) {print $3} \
                else {print $3 "/"}}' $Conf|xargs -n 1 echo)
        do
		if [ $total_squids -eq 0 ] ; then
			echo '' > ${x}swap.state
			chown squid.squid ${x}swap.state
		else
			for((i=1;i<=$total_squids;i++)) ; do
	                	echo '' > ${x}squid${i}/swap.state
				chown squid.squid ${x}squid${i}/swap.state
			done
		fi
        done
        [ -e "/data/refresh_db" ]&&rm -f /data/refresh_db/*
	if [ $total_squids -eq 0 ] ; then
		/etc/init.d/squid start 1>/dev/null #2>&1
	        Stat=$(/sbin/pidof '(squid)')
	        while [ -z "$Stat" ]
	        do
		        /etc/init.d/squid start 1>/dev/null #2>&1
	        	Stat=$(/sbin/pidof '(squid)')
	        done
	else
		/etc/init.d/flexicache start 1>/dev/null #2>&1
	        Stat=$(/sbin/pidof 'flexicache')
	        while [ -z "$Stat" ]
	        do
		        /etc/init.d/flexicache start 1>/dev/null #2>&1
	        	Stat=$(/sbin/pidof 'flexicache')
	        done

	fi
	if [ -z "$Stat" ]
	then
		echo "HTTP/1.0 400 Bad Request"
	else
		echo "HTTP/1.0 200 OK"
		/usr/local/squid/bin/refreshd -k url_verify 2>/dev/null
	fi
	#exit 0
}

funA()
{
        #no delay,check the permission
        local Arg=$1
        [ "$DEBUG" -ge "1" ]&&echo "In funA: Arg=$Arg"

	if [ "$Arg" = "ALL" ];then
		purgeCache $Arg
	else
		echo "WARNING: Option ' -A $Arg' is invalidated!"
	fi
}

opt=0
while getopts "VvhHA:" Option
do
  case $Option in
    A )
        funA $OPTARG
	opt=1
        ;;
    v|V) 
    	Version
	opt=1
	;;
    h|H) 
    	Help
	opt=1
	;;
    * ) 
    	Help
	opt=1
    ;;
  esac
done
if [ $opt -eq 0 ]
then
	Help
fi

exit 0
