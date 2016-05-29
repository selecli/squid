# load dynamic library file without interrupting FC
# by chenqi@2013/02/21

#!/bin/bash

Usage(){
	echo "Usage: $0 sofile"
}

if [ $# != 1 ];then
Usage
exit 1
fi

sofile=$1
if [ ! -f $sofile ];then
echo "No such file $sofile"
exit 1
fi

unlink /usr/local/squid/sbin/modules/`basename $sofile`
cp $sofile /usr/local/squid/sbin/modules/ 
service flexicache reload
echo "$sofile has been installed..."


