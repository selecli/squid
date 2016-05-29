#!/bin/sh

#This script used to syn and uniq cron tasks between
#files /etc/cron.d/fc-squid-all,fc-root-all and /var/
#spool/cron/squid,root. Note that it is only supported 
#by FC5.7. Any problems contact chaosheng.huang.

/usr/local/squid/bin/crond_pid.py $0 $$ 
if [ $? == 1 ] ;then exit 0;fi

###handle crontab for user root###
handle_root_crontab( )
{
	/usr/bin/crontab -l > /tmp/root.tmp.$$
	for Del in `cut -d " " -f7 /etc/cron.d/fc-root-all`
	do
		grep -v "$Del" /tmp/root.tmp.$$ >/tmp/root.tmp.bak
		mv /tmp/root.tmp.bak /tmp/root.tmp.$$
	done
	sort /tmp/root.tmp.$$ >/tmp/rootSort.tmp.$$
	uniq -f 5 /tmp/rootSort.tmp.$$ >/tmp/rootCrontab.tmp.$$
	/usr/bin/crontab -u root /tmp/rootCrontab.tmp.$$
	rm -f /tmp/root.tmp.$$
	rm -f /tmp/rootSort.tmp.$$
	rm -f /tmp/root.tmp.bak
	rm -f /tmp/rootCrontab.tmp.$$
}
###handle crontab for user squid###
handle_squid_crontab( )
{
	/usr/bin/crontab -u squid -l > /tmp/squid.tmp.$$
	for Del in `cut -d " " -f7 /etc/cron.d/fc-squid-all`
	do
		grep -v "$Del" /tmp/squid.tmp.$$ >/tmp/squid.tmp.bak
		mv /tmp/squid.tmp.bak /tmp/squid.tmp.$$
	done
	sed -i '/rotateAccess/'d /tmp/squid.tmp.$$
	sort /tmp/squid.tmp.$$ >/tmp/squidSort.tmp.$$
	uniq -f 5 /tmp/squidSort.tmp.$$ >/tmp/squidCrontab.tmp.$$
	/usr/bin/crontab -u squid /tmp/squidCrontab.tmp.$$
	rm -f /tmp/squid.tmp.$$
	rm -f /tmp/squidSort.tmp.$$
	rm -f /tmp/squid.tmp.bak
	rm -f /tmp/squidCrontab.tmp.$$
}

while read lastMTimeOfRoot
do
	if [[ "$lastMTimeOfRoot" =~ "^# [0-9]{10}.*" ]]
	then
		break
	fi
done </etc/cron.d/fc-root-all
curMTimeOfRoot=`stat -c %Y /var/spool/cron/root`
if [ ! -z "$lastMTimeOfRoot" ]
then
	lastMTimeOfRoot=`echo $lastMTimeOfRoot | cut -d " " -f 2`
	if [ $lastMTimeOfRoot != $curMTimeOfRoot ]
	then
		handle_root_crontab
		curMTimeOfRoot=`stat -c %Y /var/spool/cron/root`
		sed -i 's/# [0-9]\{10\}.*/'"# $curMTimeOfRoot"'/g' /etc/cron.d/fc-root-all
	fi
else
	handle_root_crontab
	curMTimeOfRoot=`stat -c %Y /var/spool/cron/root`
	sed -i '1i\
'"\# $curMTimeOfRoot"'' /etc/cron.d/fc-root-all
fi
	
while read lastMTimeOfSquid
do
	if [[ "$lastMTimeOfSquid" =~ "^# [0-9]{10}.*" ]]
	then
		break
	fi
done</etc/cron.d/fc-squid-all
curMTimeOfSquid=`stat -c %Y /var/spool/cron/squid`
if [ ! -z "$lastMTimeOfSquid" ]
then
	lastMTimeOfSquid=`echo $lastMTimeOfSquid | cut -d " " -f 2`
	if [ $lastMTimeOfSquid != $curMTimeOfSquid ]
	then
		handle_squid_crontab
		curMTimeOfSquid=`stat -c %Y /var/spool/cron/squid`
		sed -i 's/# [0-9]\{10\}.*/'"# $curMTimeOfSquid"'/g' /etc/cron.d/fc-squid-all
	fi
else
	handle_squid_crontab
	curMTimeOfSquid=`stat -c %Y /var/spool/cron/squid`
	sed -i '1i\
'"\# $curMTimeOfSquid"'' /etc/cron.d/fc-squid-all
fi
