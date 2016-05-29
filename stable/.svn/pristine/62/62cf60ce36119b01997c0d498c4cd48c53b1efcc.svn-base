#!/bin/bash

# Chinacache SquidDevTeam
# $Id: monitorReInstall.sh 811 2007-04-18 08:42:03Z alex $	

PWD=`pwd`
Now=${PWD}
cd monitor
make -s uninstall
make
make -s install
crontabInstall.sh
cd ${Now}/detectorig
make uninstall
make
make install
crontabInstall.sh
cd $Now
yes|cp -f ChinaCache-MIB.txt /usr/share/snmp/mibs/
