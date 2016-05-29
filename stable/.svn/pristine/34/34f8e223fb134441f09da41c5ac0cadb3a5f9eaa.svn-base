#!/bin/bash

#sed -ni '/taobao/!p' /var/named/chroot/var/named/anyhost

#echo "remove all link.data"
#rm /var/log/chinacache/link.data.*

echo "fetch data from upstream"
/usr/local/detectorig/sbin/fetch.pl

echo "do detectorig"
/usr/local/detectorig/sbin/detectorig -s
echo "analysing"
cat `ls /var/log/chinacache/link.data.* -t | head -1` | /usr/local/detectorig/sbin/analyse -d99 2>&1 | tee log

echo "domain.conf"
#grep taobao /usr/local/squid/etc/domain.conf
wc -l /usr/local/squid/etc/domain.conf

echo "link.data:"
#cat `ls /var/log/chinacache/link.data.* -t | head -1`
wc -l `ls /var/log/chinacache/link.data.* -t | head -1`

echo "anyhost:"
#grep taobao /var/named/anyhost
wc -l /var/named/anyhost
