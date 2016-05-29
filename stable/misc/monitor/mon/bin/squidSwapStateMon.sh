#!/bin/bash

#-----------------------------------------------------------------
# squidSwapStateMon.sh monitor squid swap.state file size
#                                                                 
# Developed by Qiang.guo <FlexiCache Team> 
# Report bugs to <qiang.guo@chinacache.com> 
# Copyright (c) 2007 ChinaCache Co.,Ltd. http://www.chinacache.com 
# All rights reserved. 
#-----------------------------------------------------------------
#
# Changelog:
# 2007-08-08 - created
#

# $Id: squidSwapStateMon.sh 5585 2008-11-19 05:53:41Z jyp $

Help() # {{{
{
	echo \
"give a description of the program.
Usage: foo [OPTION]...
Options:
-a, --option		another option
    --help		display this help and exit
    --version		output version information and exit
Examples:
    squidSwapStateMon.sh -a		another option
    squidSwapStateMon.sh --help		display this help and exit
    squidSwapStateMon.sh --version		output version infomation and exit
Report bugs to <qiang.guo@chinacache.com>."
} # }}}

Version( ) # {{{
{
	echo \
"squidSwapStateMon.sh 1.0.1"
	echo \
"Copyright (c) 2007 ChinaCache Co.,Ltd. http://www.chinacache.com

All rights reserved."
	echo \
"Written by Qiang.guo."
} # }}} 

# parse arguments of shell sripts {{{
while getopts ":hv" OPTION
do
	case $OPTION
		in
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
MON_POOL_DIR='../pool'
conf="../etc/monitor.conf"
swap_state_max_m=$(set -- $(grep "swap_state_max" "$conf");echo $2)
swap_state_max_k=$(((swap_state_max_m * 1024)))

#check if other porcess is runing
lockfile=${LOCKFILE-/var/lock/subsys/$(echo $(basename $0) |cut -d'.' -f1)}
/usr/bin/lockfile -1 -l 180 -r2 $lockfile 2>/dev/null
test "$?" = 0||exit 1
echo $$>$lockfile
trap 'rm -f $lockfile;rm -f $lockfile  exit 0' TERM KILL INT EXIT

error()
{
	echo "squidSwapStateSizeMBps: -1" >$MON_POOL_DIR/squidSwapStateMon.pool
	echo "squidSwapStateMaxSizeMBps: -1" >>$MON_POOL_DIR/squidSwapStateMon.pool
	exit 1
}

#check if squid.conf existed
test -r /usr/local/squid/etc/squid.conf ||error

#get the max one of swap.states 
SwapMax=$(du `grep '^cache_dir' /usr/local/squid/etc/squid.conf 2>/dev/null |awk '{print $3"/swap.state"}'` 2>/dev/null |sort -r |head -1 |awk '{print $1}')

#print current size
echo "squidSwapStateSizeMBps: ${SwapMax:--1}" >$MON_POOL_DIR/squidSwapStateMon.pool
	
#check if swap.state greater than max size
if [[ $SwapMax -gt $swap_state_max_k ]] 
then
	echo "squidSwapStateMaxSizeMBps: $SwapMax" >>$MON_POOL_DIR/squidSwapStateMon.pool
else
	echo "squidSwapStateMaxSizeMBps: 0" >>$MON_POOL_DIR/squidSwapStateMon.pool 
fi

rm -f $lockfile
  # Modeline for ViM {{{
  # vim: set ts=4:
  # vim600: fdm=marker fdl=0 fdc=3:
  # }}}
