#!/bin/bash
# Chinacache SquidDevTeam
# $Id: Step3.setup.sh 2010-04-9 by dony.zhong
# modify no purge  and kill billingd`refreshd
# add Operating for refreshd.conf fcexternal.conf /data/refresh_db;add moov_generater install
#
#
ConfDir=/usr/local/squid/etc
Conf=${ConfDir}/squid.conf
ExternalConf=${ConfDir}/fcexternal.conf
InitSquid=YES
FSTAB="/etc/fstab"
CC_log="/var/log/chinacache/"
Version=`squid -v|grep -E "Version 5"`
Moov_Mp4=`ps -ef|grep moov_generator|grep -v grep|wc -l`
#--------------------------------------------
Dir=`pwd`
#To prepare for Installation
Release=`test -f ReleaseTag.txt&&cat ReleaseTag.txt`
#Release=${Tag:-Cc}

####del fcexternal.conf of windows  ###
fcext_win=`file /usr/local/squid/etc/fcexternal.conf|grep CRLF|wc -l`
if (($fcext_win > 0))
then
  rm -f /usr/local/squid/etc/fcexternal.conf
fi

echo "Install ${Release}"
###----add refreshd.conf and fcexternal.conf by zd---------####
[ ! -s "/usr/local/squid/etc/refreshd.conf" ]&&/bin/rm -f /usr/local/squid/etc/refreshd.conf
[ ! -s "/usr/local/squid/etc/fcexternal.conf" ]&&/bin/rm -f /usr/local/squid/etc/fcexternal.conf

#backup squid.conf
if test -d $ConfDir;then
test -d /tmp/squid.$$ ||mkdir /tmp/squid.$$
yes |cp -f $ConfDir/* /tmp/squid.$$
fi
##backup crontab ---###
#/usr/bin/crontab -u root -l|wc -l
if [ `/usr/bin/crontab -u root -l|wc -l` -gt 0 ]
then
    /usr/bin/crontab -u root -l > /tmp/rootcrontab.tmp.$$
fi
if [ `/usr/bin/crontab -u squid -l|wc -l` -gt 0 ]
then
   /usr/bin/crontab -u squid -l > /tmp/squidcrontab.tmp.$$
fi

## stop service ##
/usr/bin/pkill -9 named 2> /dev/null

if [ -f /usr/local/squid/sbin/squid ];then
/usr/bin/pkill -9 squid
fi
echo -e "\n[Done]"

if [-f /usr/local/squid/bin/ca_serverd];then
/usr/bin/pkill -9 ca_serverd
fi

if [ -f /usr/local/squid/bin/billingd ];then
/usr/bin/pkill -9 billingd
fi

if [ -f /usr/local/squid/bin/refreshd ];then
/usr/bin/pkill -9 refreshd
fi

if [ -f /usr/local/squid/bin/flexicache ];then
/usr/bin/pkill -9 flexicache
fi

if [ -f /usr/local/squid/bin/lscs/sbin/nginx ]; then
/usr/bin/pkill -9 nginx
fi

/usr/bin/pkill -9 rpm

##################################################################################
#Installation starts
sleep 1
[ -e "/var/lock/rpm/transaction" ]&&rm -f /var/lock/rpm/transaction
rm -f /var/lib/rpm/__db.00*
rpm -e `rpm -q squid`
rm -Rf /usr/local/squid/sbin/*
rm -rf /usr/local/squid/bin/*
/bin/cp -rf ./usr /

chown squid:squid /usr/local/squid/etc/*
if ! test -d "/data/refresh_db";then
    mkdir -p /data/refresh_db
fi 
if [ $Moov_Mp4 -gt 0 ]
then
    killall -9 moov_generator
fi
cp -f ./usr/local/squid/bin/moov_generator /usr/local/squid/bin
chown squid:squid /data/refresh_db
#rpm -ivh  squid-2.5.12-${Release}.rpm --nodeps >/dev/null

##################################################################################
#Some useful scripts supplied by maintenance department
/bin/cp -f ./services/squid_service /etc/init.d/squid
ln -sf /etc/init.d/squid /etc/init.d/fc
/bin/cp -f ./services/squid_service /usr/local/squid/bin 
/bin/cp -f ./services/squid_service_multisquid /usr/local/squid/bin 
/bin/cp -f ./services/flexicache_service /usr/local/squid/bin 
/bin/cp -f ./service/SquidRunningMode.sh /usr/local/squid/bin
/bin/cp -f ./maint/squidlog /usr/local/squid/bin/squidlog
/bin/cp -f ./maint/gatherinfo /usr/local/squid/bin/gatherinfo
/bin/cp -f ./maint/purgeGetopts.sh /usr/local/squid/bin/purgeGetopts.sh
#/bin/cp -f ./maint/auto_ldc.sh /usr/local/squid/bin/auto_ldc.sh
/bin/cp -f ./maint/purgebdb.sh /usr/local/squid/bin/purgebdb.sh
chown squid:squid /usr/local/squid/bin/* 
##################################################################################
#For ssr
if ! test -d "/etc/ChinaCache/app.d";then
    mkdir -p /etc/ChinaCache/app.d
fi
[ -e "/etc/ChinaCache/app.d/fc.amr" ]&&/bin/rm -f /etc/ChinaCache/app.d/fc.amr
[ -e "/etc/ChinaCache/app.d/cpisfc.amr" ]&&/bin/rm -f /etc/ChinaCache/app.d/cpisfc.amr
/bin/cp -f ./ssr/cpisfc.amr /etc/ChinaCache/app.d/
##################################################################################
echo
#link purgeGetopts.sh to /usr/bin dir.
test -L /usr/bin/purgeGetopts.sh ||ln -sf /usr/local/squid/bin/purgeGetopts.sh /usr/bin

#recover old configurations.
if test -d /tmp/squid.$$;then
yes|cp -f /tmp/squid.$$/* $ConfDir/
rm -rf /tmp/squid.$$
fi

#setup cache_dir
for x in $(awk '$1 ~ /^cache_dir/ {print $3}' $Conf|xargs -n 1 echo)
 do
  test -d $x ||mkdir -p $x/coss
  Owner=`stat $x/coss|awk '$3 == "Uid:" {print $6}'`
  if [ "$Owner" != "squid)" ]
   then
    chown -R squid:squid $x/coss
  fi
 done
##delete old swap.state,create new one
#/usr/local/squid/sbin/squid -z||echo "cache_dir initialization failed!"

def_conf=/usr/local/squid/etc/upload.conf
basedirpre=$(df|awk '/proclog/ {print $6}')
[ "$basedirpre" = '/proclog' ]||basedirpre='/data/proclog'
basedir=${basedirpre}/log/squid
sed -i "/^LogBaseDir/ c\
LogBaseDir = $basedir
" $def_conf 

#you can use df to resolve which basedir to use in upload.conf
# .......not emplimentation
ifLogBaseDir=$(cat $def_conf|cut -d '#' -f1|awk -F '=' '$1 ~ /LogBaseDir/ {print $2}'| \
        awk '{print $1}'|tail -n1)
defLogBaseDir=/data/proclog/log/squid
LogBaseDir=${ifLogBaseDir:-$defLogBaseDir}
Ldir=${LogBaseDir}/archive
Backdir=${LogBaseDir}/backup
Billingdir=${LogBaseDir}/billing
test -d $Backdir ||mkdir -p $Backdir
test -d $Ldir ||mkdir -p $Ldir
test -d $Billingdir ||mkdir -p $Billingdir
test -d $CC_log ||mkdir -p $CC_log
chown -R squid:squid $LogBaseDir
chmod 777 $CC_log

#for named configuration
#yes|cp -f named.conf.squid /etc/named.conf
if test ! -f /var/named/chroot/var/named/anyhost
 then
  cat ./services/anyhost >/var/named/chroot/var/named/anyhost
 else
  cat ./services/anyhost >/var/named/chroot/var/named/anyhost.default
fi
if test ! -h /var/named/anyhost
 then
  ln -sf /var/named/chroot/var/named/anyhost /var/named/anyhost 
fi
chmod o+rx /var/named/ /var/named/chroot/ /var/named/chroot/var/
chmod o+rx /var/named/chroot/var/named/
#For SNMP monitor installation
cd $Dir
echo \$Dir=$Dir
cd monitor
make
make install
#monitor's crontab
chmod +x ./crontabInstall.sh   
./crontabInstall.sh   
cd $Dir
yes|cp -f ChinaCache-MIB.txt /usr/share/snmp/mibs/

#For the installation of original healthy status detector
cd ${Dir}/detectorig
make
make install-tar
#detectorig's crontab
chmod +x ./crontabInstall.sh
#./crontabInstall.sh
#cd $Dir
#For squid redirector installation
#cd ${Dir}/redirect
#make
#make install
cd $Dir
#For refreshd installation
#cd ${Dir}/refreshd
#/usr/bin/pkill -9 refreshd
#make
#make install
#cd ${Dir}
##-------- This section for robot.sh Installation -----------#
#cd $Dir
./robotInstall.sh
#---------------for Crontab of root -----------------------
cd $Dir
/usr/bin/crontab -u root -l > /tmp/root.tmp.$$
for Del in `cut -d " " -f7 ./root_crontab`
 do
  sed -i '/'`basename $Del`'/d' /tmp/root.tmp.$$
 done
cat root_crontab >> /tmp/root.tmp.$$
sort -b /tmp/root.tmp.$$ |uniq >/tmp/rootCrontab.tmp.$$
/usr/bin/crontab -u root /tmp/rootCrontab.tmp.$$
rm -f /tmp/root.tmp.$$
rm -f /tmp/rootCrontab.tmp.$$
#---------------for logrotate of squid logs ---------------
touch /tmp/hollow_file.tmp.$$
/usr/bin/crontab -u squid /tmp/hollow_file.tmp.$$
rm -f /tmp/hollow_file.tmp.$$
yes |cp ./rotate.squid /etc/logrotate.d/squid
cd $Dir
/usr/bin/crontab -u squid -l >/tmp/squid.tmp.$$
#change by liwei
for Del in `cut -d " " -f7 squid_crontab`
 do
  sed -i '/'`basename $Del`'/d' /tmp/squid.tmp.$$
 done
#cat squid_crontab >> /tmp/squid.tmp.$$
#change by liwei
cat squid_crontab >> /tmp/squid.tmp.$$
sort -b /tmp/squid.tmp.$$|uniq>/tmp/squidCrontab.tmp.$$
sed -i '/rotateBilling/d' /tmp/squidCrontab.tmp.$$
sed -i '/rotateReceipt/d' /tmp/squidCrontab.tmp.$$
/usr/bin/crontab -u squid /tmp/squidCrontab.tmp.$$
rm -f /tmp/squid.tmp.$$
rm -f /tmp/squidCrontab.tmp.$$
#add by chenqi, for FC7
sed -i '1,$s/.*flexicache.*//g' /var/spool/cron/root    
sed -i '/^$/d' /var/spool/cron/root
FC6ConfigFile=/usr/local/squid/etc/flexicache.conf
[ ! -f $FC6ConfigFile ] && mv $FC6ConfigFile.default $FC6ConfigFile
#add end
#---------------Eof Crontab of squid ---------------------
[ "x$InitSquid" = "xYES" ]&&./initiatingSquid.sh 

#-------------- Other useful scripts---------------------
yes|cp ./monitor.sh /usr/local/squid/bin/monitor.sh
#-----------delete deprecated configure entry negative_ttl -------
sed -i '/^[[:space:]]*negative_ttl/ d' /usr/local/squid/etc/squid.conf

#-------------- For preloadmanger installation--------------------
#cd $Dir
#if test -d preloadmanger
# then
#  cd preloadmanger
#  make&&make install
#  cd $Dir
#fi
#-------------- For preload installation--------------------
#cd $Dir
#if test -d preload
# then
#  cd preload
#  make&&make install
#  cd $Dir
#fi
#------------- For TCP settings  ------------------------------
TMP=`mktemp`
cat << 'EOD' > $TMP
# Modify some TCP_ARGUMENTS to improve squid performance
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_max_syn_backlog = 8192
net.core.somaxconn = 1024
net.ipv4.tcp_fin_timeout = 10
net.ipv4.tcp_synack_retries = 3
net.ipv4.tcp_syn_retries = 3
# End of modify TCP_ARGUMENTS
EOD

echo 8192 > /proc/sys/net/ipv4/tcp_max_syn_backlog
echo 1 > /proc/sys/net/ipv4/tcp_syncookies
echo 1024 > /proc/sys/net/core/somaxconn
echo 10 > /proc/sys/net/ipv4/tcp_fin_timeout
echo 3 > /proc/sys/net/ipv4/tcp_synack_retries
echo 3 > /proc/sys/net/ipv4/tcp_syn_retries
SYSCTL_CONF=/etc/sysctl.conf
sed -i '/Modify some TCP_ARGUMENTS/,/End of modify TCP_ARGUMENTS/d' $SYSCTL_CONF
cat $TMP >> $SYSCTL_CONF
rm -f $TMP
chown -R squid:squid /usr/local/squid
chmod +x /usr/local/squid/bin/*
[ -e "$ExternalConf" ]&&sed -i 's/http:\/\/.*:8080/http:\/\/61.135.209.134:8080/g' $ExternalConf
sed -i '/^start\(\)/i\
ulimit -n 500000' /etc/init.d/named

#see case 2894
test -f /usr/local/squid/etc/redirect.conf \
|| touch /usr/local/squid/etc/redirect.conf;chown squid:squid /usr/local/squid/etc/redirect.conf

service named start &
/usr/local/bin/robot.sh

if [ -e "/tmp/rootcrontab.tmp.$$" ]; then
    /usr/bin/crontab -u root /tmp/rootcrontab.tmp.$$
    /bin/rm -rf /tmp/rootcrontab.tmp.$$
fi

service squid start &
