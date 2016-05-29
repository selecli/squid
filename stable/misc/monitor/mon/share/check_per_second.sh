#!/bin/sh

cd /monitor/share

#check system load
/monitor/bin/loadMon.pl

#check system connection
/monitor/bin/netConnMon.pl

#checking squid response 
/monitor/bin/squidResponseMon.pl

