#!/bin/bash

# Chinacache SquidDevTeam
# $Id: crontabInstall.sh 64 2006-11-10 14:45:00 zengluojun$

crontab -u root -l > /tmp/check_per_minute.tmp.$$
Installed=`grep 'check_per_minute.sh' /tmp/check_per_minute.tmp.$$|grep -v '^#'`
if [ -z "$Installed" ];then
echo "*/1 * * * * /monitor/share/check_per_minute.sh >/dev/null 2>&1" >> /tmp/check_per_minute.tmp.$$
fi
crontab -u root /tmp/check_per_minute.tmp.$$

crontab -u root -l > /tmp/check_per_minute.tmp.$$
Installed=`grep 'check_per_minute.sh' /tmp/check_per_minute.tmp.$$|grep -v '^#'`
if [ -z "$Installed" ];then
        echo "Failed!"
        exit 1
else
        echo "check_per_minute.sh installation is completed!"
        exit 0
fi

rm -f /tmp/check_per_minute.tmp.$$
