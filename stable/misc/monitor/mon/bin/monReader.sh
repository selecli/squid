#!/bin/bash

#-----------------------------------------------------------------
# monReader.sh
#                                                                 
# Developed by Qiang.guo <FlexiCache Team> 
# Report bugs to <qiang.guo@chinacache.com> 
# Copyright (c) 2007 ChinaCache Co.,Ltd. http://www.chinacache.com 
# All rights reserved. 
#-----------------------------------------------------------------
#
# Changelog:
# 2007-08-06 - created
#

# $Id: monReader.sh 5585 2008-11-19 05:53:41Z jyp $

Help() # {{{
{
	echo \
"give a description of the program.
Usage: foo [OPTION]...
Options:
    -m		monitors name and numbers of arguments
    --help		display this help and exit
    --version		output version information and exit
Examples:
    monReader.sh -m "load.3"		input monitor's name and argument's number
    monReader.sh --help		display this help and exit
    monReader.sh --version		output version infomation and exit
Report bugs to <qiang.guo@chinacache.com>."
} # }}}

Version( ) # {{{
{
	echo \
"monReader.sh 1.0.0"
	echo \
"Copyright (c) 2007 ChinaCache Co.,Ltd. http://www.chinacache.com

All rights reserved."
	echo \
"Written by Qiang.guo."
} # }}} 

# parse arguments of shell sripts {{{
while getopts ":hvm:n:" OPTION
do
	case $OPTION
		in
		m) PoolName=$OPTARG # the monitor name and numbers of arguments
		;;
		h) Help # display help and exit
			exit 0
		;;
		v) Version # output version infomation and exit
			exit 0
		;;
		*)
		;;
	esac
done # }}}

################ start here #####################
basedir="/monitor"

failed_check ()
{
LIMIT=$1
for ((a=1; a <= LIMIT ; a++))
do
	echo "-1"
done
}
if test "$PoolName"
then
for monitor in $PoolName
do
	pool=$(echo $monitor |cut -d"." -f1)
	num=$(echo $monitor |cut -d"." -f2)
	Date=$(date +%s)
	if [ ! -e "${basedir}/pool/${pool}Mon.pool" ]
	then
		failed_check $num 
		continue
	fi
	Modified=$(stat -c %Y "${basedir}/pool/${pool}Mon.pool")
	if (( ($Date - $Modified) < 300))
	then
		while read x
		do
			set -- $x
			shift
			echo $@
		done <${basedir}/pool/${pool}Mon.pool
	else	
		failed_check $num
	fi
done
fi
# Modeline for ViM {{{
# vim: set ts=4:
# vim600: fdm=marker fdl=0 fdc=3:
# }}}
