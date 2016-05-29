#!/bin/bash

[ -f usr/local/squid/etc/IP2Location.h ] && cp ./usr/local/squid/etc/IP2Location.h /usr/local/include/
[ -f usr/local/squid/etc/libIP2Location.so ] && cp ./usr/local/squid/etc/libIP2Location.so /lib/

/bin/bash ./Step1.prepare.sh && . /etc/profile
/bin/bash ./Step3.setup.sh
/bin/bash ./Step5.tunningServices.sh

ver=`/usr/local/squid/sbin/squid -v|head -1`
echo "`date`"" Installed "$ver >>/var/log/chinacache/FlexiCacheInstall.log
echo "Installation is completed!"
