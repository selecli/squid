#!/bin/bash

#delete FC config in rc.local.
sed -e 's/ulimit.*$//' -e 's|.*ip_local_port_range||' -e '/^$/ d' /etc/rc.d/rc.local 

#delete old configuration
sed -i '/# add for squid/,/# end of squid/ d' /etc/profile 

source /etc/profile

echo " Modify /etc/resolv.conf, add DNS server"

resolv="/etc/resolv.conf"

cat <<EOF > $resolv
# add DNS server for default 
nameserver 202.106.0.20 
# end of default
EOF

#stop squid process
if [ -x /usr/local/squid/sbin/squid ];then
/usr/local/squid/sbin/squid -k kill
fi
service squid stop 1>/dev/null

#stop refreshd billingd
/usr/bin/killall -9 refreshd
/usr/bin/killall -9 billingd

##del /usr/local/squid/bin
test -d /usr/local/squid/bin&&rm -rf /usr/local/squid/bin
test -d /usr/local/squid/sbin&&rm -rf /usr/local/squid/sbin
test -d /data/proclog/log/squid&&rm -rf /data/proclog/log/squid
test -d /data/proclog/log/refreshd&&rm -rf /data/proclog/log/refreshd
test -d /data/refresh_db&&rm -rf /data/refresh_db
test -d /var/log/chinacache&&rm -rf /data/log/chincache
test -f /etc/init.d/squid/squid_service&&rm -f /etc/init.d/squid

Moov_Mp4=`ps -ef|grep moov_generator|grep -v grep|wc -l`

if [ $Moov_Mp4 -gt 0 ]
then
    /usr/bin/killall -9 moov_generator
fi

FCVersion=`grep 'echo \"FlexiCache Version: \`/usr/local/squid/s' \
/etc/rc.d/rc.local`

if test -z "$FCVersion";then
sed -i '/FlexiCache Version/'d /etc/rc.d/rc.local
fi
sed -i '/FlexiCache Version/'d /etc/motd

#for httpd configuration
#if test -f /etc/httpd/conf/httpd.conf;then
#sed -i 's/^Listen.*$/Listen 80/g' /etc/httpd/conf/httpd.conf
#lll=`grep "Listen " /etc/httpd/conf/httpd.conf|grep -v "#"`
#service httpd restart
#echo "Apache server now is listening $lll port"
#echo " End of prepare squid setup enviroment"
#else 
#	echo "/etc/httpd/conf/httpd.conf doesn't exist,check your httpd service status"
#fi

#if test -f /usr/local/apache2/conf/httpd.conf;then
#yes|cp -f httpd.conf.squid /usr/local/apache2/conf/httpd.conf
#lll=`grep "Listen " /usr/local/apache2/conf/httpd.conf|grep -v "#"`
#service httpd reload
#echo "Apache server now is listening $lll port"
#echo " End of prepare squid setup enviroment"
#else
#   echo "/usr/local/apache2/conf/httpd.conf doesn't exist,check your httpd service status"
#fi

#Uninstallation
#----------------------------------------------------------------------
ConfDir=/usr/local/squid/etc
Conf=${ConfDir}/squid.conf
#--------------------------------------------
Dir=`pwd`
 
#To prepare for Uninstallation

#backup squid.conf
if test -d $ConfDir;then
test -d /tmp/squid.$$ ||mkdir /tmp/squid.$$
yes |cp -f $ConfDir/* /tmp/squid.$$
fi

#rm -rf cache_dir

#for x in $(awk '$1 ~ /^cache_dir/ {print $3}' $Conf|xargs -n 1 echo)
#    do
#                rm -rf $x
#    done

def_conf=/usr/local/squid/etc/upload.conf
basedirpre=$(df|awk '/proclog/ {print $6}')
[ "$basedirpre" = '/proclog' ]||basedirpre='/data/proclog'
basedir=${basedirpre}/log/squid
sed -i "/^LogBaseDir/ c\
LogBaseDir = $basedir
" $def_conf 

ifLogBaseDir=$(cat $def_conf|cut -d '#' -f1|awk -F '=' '$1 ~ /LogBaseDir/ {print $2}'| \
        awk '{print $1}'|tail -n1)
defLogBaseDir=/data/proclog/log/squid
LogBaseDir=${ifLogBaseDir:-$defLogBaseDir}
rm -rf $LogBaseDir

#for named configuration
if test -f /var/named/chroot/var/named/anyhost;then
	rm -f /var/named/chroot/var/named/anyhost
	rm -f /var/named/anyhost
fi


#For SNMP monitor installation
cd $Dir
if test -d monitor;then
	cd monitor
	make uninstall
	#for monitor's crontab
	./crontabUninstall.sh
	cd $Dir
else
	test -d /monitor&&rm -rf /monitor
fi

#For the installation of original healthy status detector
cd $Dir
if test -d detectorig;then
        cd detectorig
        make uninstall
        #for monitor's crontab
        ./crontabUninstall.sh
        cd $Dir
else
        test -d /usr/local/detectorig/&&rm -rf /usr/local/detectorig/
fi

#-------- This section for robot.sh Installation -----------#
cd $Dir

if test -x robotUninstall.sh;then
	./robotUninstall.sh
else
	rm -f /usr/local/bin/robot.sh
	if test -f /usr/local/bin/robot.sh;then
        	echo "Error:Unable to delete /usr/local/bin/robot.sh"
	fi
	crontab -u root -l > /tmp/robot.tmp.$$
	sed -i '/'robot.sh'/d' /tmp/robot.tmp.$$

	crontab -u root /tmp/robot.tmp.$$

	Installed=`grep 'robot.sh' /tmp/robot.tmp.$$|grep -v '^#'`
	if [ -z "$Installed" ];then
        	echo "Unintallation is completed"
	else
        	echo "Error:Unable to change crontab! unintall it by hands!"
	fi
	rm -f /tmp/robot.tmp.$$
fi
#-----------------------------------------------------------
#---------------Eof Crontab of root ---------------------

rpm -qa |grep squid|xargs -n 1 rpm -e

#remove purgeGetopts.sh 
test -L /usr/bin/purgeGetopts.sh && rm -f /usr/bin/purgeGetopts.sh
test -f /usr/local/squid/bin/purgeGetopts.sh&&rm -f /usr/local/squid/bin/purgeGetopts.sh

#recover old configurations.
if test -d /tmp/squid.$$;then
crontab -u root -l >/tmp/squid.$$/crontab_root.bak
crontab -u squid -l >/tmp/squid.$$/crontab_squid.bak
test -d $ConfDir||mkdir -p $ConfDir
yes|cp -f /tmp/squid.$$/* $ConfDir/
rm -rf /tmp/squid.$$
fi


crontab -u root -r
crontab -u squid -r
######################################################################
#Modified by YangNan at 2009-6-30
#Modified by dong.zhong at 2011-1-20
[ -e "/etc/ChinaCache/app.d/fc.amr" ]&&/bin/rm -f /etc/ChinaCache/app.d/fc.amr
[ -e "/etc/ChinaCache/app.d/cpisfc.amr" ]&&/bin/rm -f /etc/ChinaCache/app.d/cpisfc.amr
######################################################################
echo FlexiCache Uninstalled.
exit 0

