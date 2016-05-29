#!/bin/bash
rm -f /usr/local/bin/robot.sh
if test -f /usr/local/bin/robot.sh;then
	echo "Error:Unable to delete /usr/local/bin/robot.sh"
	exit 1
fi
crontab -u root -l > /tmp/robot.tmp.$$
sed -i '/'robot.sh'/d' /tmp/robot.tmp.$$

crontab -u root /tmp/robot.tmp.$$

Installed=`grep 'robot.sh' /tmp/robot.tmp.$$|grep -v '^#'`
if [ -z "$Installed" ];then
        echo "Unintallation is completed"
        exit 0
else
	echo "Error:Unable to change crontab! unintall it by hands!"
        exit 1
fi

rm -f /tmp/robot.tmp.$$
