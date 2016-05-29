#!/bin/bash

# Author: zhaobin
# Version: 6.0.2
# Function: purge FC cache objects
# History:
#     2013/05/31  zhaobin refactoring

# ===================== functions ===========================

# brief:
#        show usage infomation
# params:
#        null
# return:
#        null
Usage()
{
    echo "Usage: $0 [options]"
    echo "  -A ALL  purge cache";
    echo "  -h      show help message";
    echo "  -v      show version";
    exit $E_OPTERROR
}

# brief:
#        show version infomation
# params:
#        null
# return:
#        null
Version()
{
    echo "$0 Version:6.0.2 Last modified at 2013-05-31"
}

# brief:
#        dejudge FC running mode
# params:
#        null
# return:
#        0    not running copy
#        1    single squid
#        2    multiple squid
getFCMode() 
{
    ret=$(pidof flexicache)

    if [ "$ret" != "" ]
    then
        return 2
    fi

    ret=$(pidof squid)

    if [ "$ret" != "" ]
    then
        return 1
    fi

    return 0
}

# brief:
#        start FC according to the params
# params:
#        1    single squid
#        2    multiple squid
# return:
#        null
startFC()
{
    mode=$1
    if [ $mode -eq 2 ]
    then
        /etc/init.d/flexicache start multiple
		getFCMode
		res=$?
		if  [ "$res" -eq 2 ]
		then
			echo "Purge Success, Start FC Success" 
		else
			echo "Purge Success, Start FC Fail"
		fi
    else
        /etc/init.d/squid start 
		getFCMode
		ret=$?
		if  [ "$res" -eq 1 ]
		then
			echo "Purge Success, Start FC Success"
		else
			echo "Purge Success, Start FC Fail"
		fi
    fi

    /usr/local/squid/bin/refreshd -k url_verify
}

# brief:
#        stop FC according to the params
# params:
#        1    single squid
#        2    multiple squid
# return:
#        null
stopFC ()
{
    mode=$1
    if [ $mode -eq 2 ]
    then
        pkill -9 flexicache
        /usr/local/squid/bin/lscs/sbin/nginx -s stop >/dev/null 2>&1
        pkill -9 -f fc-cache
    else
        pkill -9 squid
    fi

    pkill -9 refreshd >/dev/null 2>&1
}

# brief:
#        clear cache file according to the params
# params:
#        1    single squid
#        2    multiple squid
# return:
#        null
clearCache()
{    
    mode=$1
    total_squid=0
    if [ $mode -eq 2 -a -f "$fcConf" ]
    then
        total_squid=$(awk '/^ *cache_processes/{print $2}' $fcConf)
        if [ -z "$total_squid" ]
        then
            total_squid=0
        fi
    fi

    for dir in $(awk '$1 ~ /^cache_dir/ {if ($3 ~ /.*\/$/) {print $3} else {print $3 "/"}}' $squidConf|xargs -n 1 echo)
    do
        if [ $total_squid -eq 0 ] ; then
            echo '' > ${dir}swap.state
            chown squid.squid ${dir}swap.state
        else
            for ((i=1;i<=$total_squid;i++)) ; do
                echo '' > ${dir}squid${i}/swap.state
                chown squid.squid ${dir}squid${i}/swap.state
            done
        fi
    done

    if [ -e "/data/refresh_db" ]
    then
        rm -f /data/refresh_db/*
    fi
}    

# ===================== main ===========================
opt=0
squidConf=/usr/local/squid/etc/squid.conf
fcConf=/usr/local/squid/etc/flexicache.conf
E_OPTERROR=65

while getopts "VvhHA:" Option
do
    case $Option in
        A )
            if [ $OPTARG = "ALL" ]
            then
                opt=1
                getFCMode
                res=$?
                if  [ "$res" -eq 0 ]
                then 
                    clearCache 1
                    clearCache 2
                else
                    stopFC $res
                    clearCache $res
                    startFC $res
                fi
            else
                echo "WARNING: Option ' -A $OPTARG' is invalidated!"
                opt=0
            fi
        ;;
        v|V) 
            Version
            opt=1
        ;;
        h|H) 
            Usage
            opt=1
        ;;
        * ) 
            Usage
            opt=1
        ;;
    esac
done

if [ $opt -eq 0 ]
then
    Usage
fi

exit 0
