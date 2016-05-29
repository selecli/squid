#!/bin/bash

crontab -u root -l > /tmp/check_per_minute.tmp.$$
sed -i '/'check_per_minute.sh'/d' /tmp/check_per_minute.tmp.$$

crontab -u root /tmp/check_per_minute.tmp.$$

Installed=`grep 'check_per_minute.sh' /tmp/check_per_minute.tmp.$$|grep -v '^#'`
if [ -z "$Installed" ];then
        echo "Unintallation is completed"
        exit 0
else
	echo "Error:Unable to change crontab! unintall it by hands!"
        exit 1
fi

rm -f /tmp/check_per_minute.tmp.$$
