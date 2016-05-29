#!/bin/bash
SquidVersion=`/usr/local/squid/sbin/squid -v|awk '$3 ~/Version/ {print $4}' `
SquidSetupVersion=`grep squidsetup /usr/local/squid/etc/squidsetupReleaseNotes.txt`

if [ -n "$SquidVersion" ];then
        echo "$SquidVersion"
else
        echo "-1"
fi

if [ -n "$SquidSetupVersion" ];then
        echo "$SquidSetupVersion"
else
        echo "-1"
fi
