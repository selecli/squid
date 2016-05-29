#!/bin/bash

#it is not neccessary to chkconfig off these services and we have no privilegied to do this

#services=`ls -1 /etc/rc3.d|grep S|grep -v sysstat|grep -v network|grep -v syslog|grep -v irqbalance|grep -v sshd \
#|grep -v syslog|grep -v irqbalance|grep -v sshd|grep -v crond|grep -v anacron|grep -v atd|grep -v snmpd \
#|grep -v local|grep -v named|grep -v squid|grep -v iptables|grep -v cc_ng|grep -v ssd`
#
#echo turn off services as follows:
#echo
#sleep 2
#for y in $services
#do
#ycut=`echo $y|cut -b 4-`
#test -n $ycut&&service $ycut stop&&chkconfig $ycut off
#done

#sleep 1
##############################################################
echo auto-starting these services as follows at bootup:
for x in \
sysstat \
network \
syslog \
irqbalance \
sshd \
crond \
anacron \
atd \
snmpd \
named \
squid \
flexicache
do
	if [ -f "/etc/init.d/$x" ] ; then
		chkconfig $x on
		stat=`env LC_ALL=en_US.UTF-8 chkconfig --list $x|awk '{print $5}'|cut -d':' -f2`
		test "$stat" = "on"&& echo -e \
		"\033[32m $x daemon will start at bootup"&& \
		echo -e "\033[0m " || \
		echo -e  \
		"\033[31m $x daemon does not start at bootup"&&echo -e "\033[0m "
		sleep 1
	fi
done
echo -e "\033[32m local program will always start at bootup"&& \
echo -e "\033[0m "
echo "FlexiCache Version: `/usr/local/squid/sbin/squid -v |grep Version |awk '{print $4}'`" > /etc/motd
