#!/bin/bash

# Chinacache SquidDevTeam
# $Id: robotInstall.sh 12736 2012-02-06 13:53:54Z chaosheng.huang $

test -d /usr/local/bin/||mkdir -p /usr/local/bin
cp -f robot.sh /usr/local/bin/robot.sh
chmod +x /usr/local/bin/robot.sh

if test -f /usr/local/bin/robot.sh;then
	:
else
	echo "Error: /usr/local/bin/robot.sh does not exist!"
	exit 1
fi
if test ! -x /usr/local/bin/robot.sh;then
	echo "Error: /usr/local/bin/robot.sh is not executable"
	exit 1
fi

crontab -u root -l > /tmp/robot.tmp.$$
Installed=`grep 'robot.sh' /tmp/robot.tmp.$$|grep -v '^#'`
if [ -z "$Installed" ];then
echo "*/3 * * * * /usr/local/bin/robot.sh >/dev/null 2>&1" >> /tmp/robot.tmp.$$
fi
crontab -u root /tmp/robot.tmp.$$

crontab -u root -l > /tmp/robot.tmp.$$
Installed=`grep 'robot.sh' /tmp/robot.tmp.$$|grep -v '^#'`
if [ -z "$Installed" ];then
	echo "Failed!"
	exit 1
else
	echo "robot.sh installation is completed!"
	exit 0
fi

rm -f /tmp/robot.tmp.$$
