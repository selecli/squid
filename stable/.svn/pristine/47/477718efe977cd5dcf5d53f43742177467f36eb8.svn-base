#!/bin/bash
# Chinacache SquidDevTeam
# $Id$	

/usr/local/squid/bin/crond_pid.py $0 $$ 
if [ $? == 1 ] ;then exit 0;fi

cd /monitor/share
/monitor/bin/diskStatusCollector.sh >/monitor/pool/diskStatus.pool
/monitor/bin/squidResponseMon.pl

