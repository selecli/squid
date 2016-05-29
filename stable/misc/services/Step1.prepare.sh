#!/bin/bash
# Chinacache SquidDevTeam
# $Id: Step1.prepare.sh 17113 2013-03-22 03:13:18Z chenqi $
# 1. step1-------------------------------
# /usr/include/bits/typesizes.h  
#/* Number of descriptors that can fit in an `fd_set'.  */
# #define __FD_SETSIZE            1024 * 340
sed -i "/__FD_SETSIZE/ c\
#define __FD_SETSIZE            348160
" /usr/include/bits/typesizes.h
# 2. step2----------------------------
#del old ones.
sed -e 's/ulimit.*$//' -e 's|.*ip_local_port_range||' -e '/^$/ d' /etc/rc.d/rc.local 
# increase the process file-descriptor limit in the same shell in which you will configure and compile Squid:
FD=`grep 'ulimit -Hn 348160' /etc/rc.d/rc.local`
if test -z "$FD";then
echo 'ulimit -Hn 348160' >>/etc/rc.d/rc.local
fi

###----add squid user and group ---###
if grep -w "squid" /etc/group > /dev/null; then
usleep 1
else
/usr/sbin/groupadd squid
fi
if grep -w "squid" /etc/passwd > /dev/null; then
usleep 1
else
/usr/sbin/useradd -d /var/spool/squid -g squid -s /sbin/nologin squid
fi

###----add squid user and group ---###



#3 increasing  Ephemeral port
#  echo "1024 61000" > /proc/sys/net/ipv4/ip_local_port_range
PortRange=`grep 'echo "1024 61000" > /proc/sys/net/ipv4/ip_local_port_range' \
/etc/rc.d/rc.local`
if test -z "$PortRange";then
echo 'echo "1024 61000" > /proc/sys/net/ipv4/ip_local_port_range' >>/etc/rc.d/rc.local
fi

###################################################################################################################################
#Modified by yang nan
#if [ -e "/usr/local/squid/etc/squid.conf" ]
# then
#  sed -i '/^cc_compress_delimiter/d' /usr/local/squid/etc/squid.conf
#  echo "cc_compress_delimiter @@@" >> /usr/local/squid/etc/squid.conf
#fi
sed -i '/CheckDetection.sh/d' /etc/rc.d/rc.local
#echo "echo \"FlexiCache Version: \`/usr/local/squid/sbin/squid -v |grep Version |awk '{print \$4}'\`\" > /etc/motd" >>/etc/rc.d/rc.local

FCVersion=`grep 'echo \"FlexiCache Version: \`/usr/local/squid/s' \
/etc/rc.d/rc.local`
if test -z "$FCVersion";then
echo "echo \"FlexiCache Version: \`/usr/local/squid/sbin/squid -v |grep Version |awk '{print \$4}'\`\" > /etc/motd" >>/etc/rc.d/rc.local
fi


if [[ -x /etc/init.d/ssd ]]
then
        chkconfig --add ssd
        chkconfig --level 2345 ssd on
fi
###################################################################################################################################

#delete old configuration
sed -i '/# add for squid/,/# end of squid/ d' /etc/profile 

profile="/etc/profile"
profile_stat=`grep "ulimit -HSn 348160" $profile`
platform=`uname -m`
if  test -z "$profile_stat" 2>/dev/null 
then
if test $platform == "x86_64"
then
cat <<EOF >> $profile

# add for squid program compile
export CFLAGS="-O2  -mcpu=x86_64 -march=x86_64"
PATH=.:/usr/local/squid/sbin:/usr/local/squid/bin:\$PATH
export PATH

# Increase linux file descriptor
ulimit -HSn 344800 
# end of squid 
EOF
else
cat <<EOF >> $profile

# add for squid program compile
export CFLAGS="-O2  -mcpu=i686 -march=i686 "
PATH=.:/usr/local/squid/sbin:/usr/local/squid/bin:\$PATH
export PATH

# Increase linux file descriptor
ulimit -HSn 344800 
# end of squid 
EOF
fi
fi

source /etc/profile

echo " Modify /etc/sysctl.conf, add network related parameters"
sysctl="/etc/sysctl.conf"
sys_stat=`grep "net.ipv4.tcp_max_tw_buckets = 1800000" $sysctl`
if test -z $sys_stat 2>/dev/null 
then
cat <<EOF >> $sysctl

# add for squid program
# Increase the tcp-time-wait buckets pool size
net.ipv4.tcp_max_tw_buckets = 1800000
net.ipv4.tcp_max_syn_backlog = 8192
#2nd try
kernel.msgmnb=8192
kernel.msgmni=40
kernel.msgmax=8192
kernel.shmall=2097152
kernel.shmmni=32
kernel.shmmax=16777216
# end of squid

EOF
fi

###--add chinacache log dir ---##
if [ -d /var/log/chinacache ]
then
        /bin/chown squid.squid -R  /var/log/chinacache/
else
        /bin/mkdir -p /var/log/chinacache
        /bin/chown squid.squid -R  /var/log/chinacache/
fi
####-------chinacache log -----##


echo '##################################################################'
echo "#Step2:   To enable your environment by running 'source /etc/profile' #"
echo '##################################################################'
exit
