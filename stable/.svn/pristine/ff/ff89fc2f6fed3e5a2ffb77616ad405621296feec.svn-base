#####################################################
#!/bin/bash
# This script is used to switch FC running mode between multi-squid mode and single-squid mode.
# Author: chenqi
# Group: CSP/GSP
# Date: 2012/08/21
#####################################################

APPPATH="/usr/local/squid/bin/"

Usage(){
	echo "Usage: $0 [-psmf]"
	echo " -p	Get FC current running mode, multi-squid or signle-squid."
	echo " -s	Set FC Running in Single-squid mode."
	echo " -m	Set FC Running in Multi-squid mode."
	echo " -f	Force Switch squid mode!"
}

if [ $# -ne 1 ];then
	Usage
	exit 1	
fi

mode=3
force=0
while getopts "psmhf" options;do
case $options in
	p)	/etc/init.d/fc status;;
	s)      /etc/init.d/fc switch single;;
	m)      /etc/init.d/fc switch multiple;;
	f)	force=1;;
	h)      Usage;exit 0;;
esac
done


#####################################################
# This function tells us FC running mode
# if FC not have running copy, return 0
# if FC running multi-squid copy, return 2
# if FC running single-squid copy, return 1
#####################################################

#TellMode() {
#        ret=$(pidof flexicache)
#        if [ "$ret" != "" ]
#        then
#            return 2
#        fi
#        ret=$(pidof squid)
#        if [ "$ret" != "" ]
#        then
#            return 1
#        fi
#        return 0
#}
#
##################################################
## This function checks whether this script make effect
##################################################
#TellResult(){
#	TellMode
#	result=$?
#	echo "The Result is: $result"
#	if [ $1 -eq $result ];then
#		echo "switched success..."
#	else
#		echo "switched failed..."
#	fi
#}
#
#MarkMode(){
#	tmp=$(cat /etc/profile | grep ^FCMODE | wc -l)
#	if [ $tmp -eq 1 ];then
#		if [ $1 -eq 1 ];then
#			sed -i 's/^FCMODE=.*/FCMODE=single/g' /etc/profile
#		elif [ $1 -eq 2 ];then
#			sed -i 's/^FCMODE=.*/FCMODE=multi/g' /etc/profile
#		fi
#	else
#		if [ $1 -eq 1 ];then
#			echo 'FCMODE=single' >> /etc/profile
#		elif [ $1 -eq 2 ];then
#			echo 'FCMODE=multi' >> /etc/profile
#		fi
#	fi
#
#	source /etc/profile
#}
#
#
#if [ $mode -eq 0 ];then
#	TellMode
#	result=$?
#	if [ $result -eq 0 ];then echo "FC: ERROR: No running copy"
#	elif [ $result -eq 1 ];then echo "FC: Single Squid running"
#	elif [ $result -eq 2 ];then echo "FC: Multiple Squid running"
#	fi
#
#elif  [ $mode -eq 1 ];then
#	echo "FC will run in single-squid mode..."
#	TellMode
#	result=$?
#	if [ $result -eq 1 -a $force -eq 0 ];then echo "FC: already running in single-squid mode..."; exit 0;fi
#	if [ $result -eq 1 -a $force -eq 1 ];then service squid stop; fi
#	if [ $result -eq 2 ];then service flexicache stop; fi
#
#	cd ${APPPATH}
#	[ -f /etc/init.d/flexicache ] && rm -f /etc/init.d/flexicache
#	[ -f /etc/init.d/fc ] && rm -f /etc/init.d/fc
#	[ -f /etc/init.d/squid ] && rm -f /etc/init.d/squid
#	cp -f ./squid_service /etc/init.d/squid
#	ln -s /etc/init.d/squid /etc/init.d/flexicache
#
#	sed -i '1,$s/.*flexicache.*//g' /var/spool/cron/root	
#	sed -i '/^$/d' /var/spool/cron/root
#
#	/sbin/chkconfig flexicache off
#	/sbin/chkconfig --levels 235 squid on
#	/sbin/service squid start
#
#	sleep 3
#	MarkMode 1
#	TellResult 1
#
#elif [ $mode -eq 2 ];then
#	echo "FC will run in multiple-squid mode..."
#
#	TellMode	
#	result=$?
#	if [ $result -eq 2  -a $force -eq 0 ];then echo "FC: already running in multiple-squid mode..."; exit 0;fi
#	if [ $result -eq 2  -a $force -eq 1 ];then service flexicache stop; fi
#	if [ $result -eq 1 ];then service squid stop; fi
#
#	cd ${APPPATH}
#	[ -f /etc/init.d/flexicache ] && rm -f /etc/init.d/flexicache
#	[ -f /etc/init.d/fc ] && rm -f /etc/init.d/fc
#	[ -f /etc/init.d/squid ] && rm -f /etc/init.d/squid
#	cp -f ./flexicache_service /etc/init.d/flexicache
#	cp -f ./squid_service_multisquid /etc/init.d/squid
#
#	tmp=$(/bin/cat /var/spool/cron/root | /bin/grep "flexicache -k rotatelog")
#	if [ "$tmp" = "0 0 * * * /usr/local/flexicache/sbin/flexicache -k rotatelog >/dev/null 2>&1" ];then
#		echo "0 0 * * * /usr/local/flexicache/sbin/flexicache -k rotatelog >/dev/null 2>&1 has existed"
#	else
#		echo "0 0 * * * /usr/local/flexicache/sbin/flexicache -k rotatelog >/dev/null 2>&1" >> /var/spool/cron/root 
#	fi
#
#	/sbin/chkconfig squid off 
#	/sbin/chkconfig --levels 235 flexicache on
#	/sbin/service flexicache start
#
#	sleep 3
#	MarkMode 2
#	TellResult 2
#else
#	Usage
#fi
#
#exit 0
