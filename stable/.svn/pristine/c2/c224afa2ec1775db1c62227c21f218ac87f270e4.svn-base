Name: FC 
Summary: ChinaCache FlexiCache 
Version: 7.0 
Release: R
Vendor: CSP-GSP 
License: TBD
Group: Enterprise/ChinaCache
BuildRoot: /tmp/fc-build-root
URL: http://www.chinacache.com
Provides: 16488_std
Exclusiveos: linux

%description
Distribution: ChinaCache 

%define _builddir           .
%define _rpmdir             .
%define _build_name_fmt     %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm
%define __os_install_post   /usr/lib/rpm/brp-compress; echo 'Not stripping.'

%prep

%build
revision=`echo %{provides} | cut -d "_" -f1`
versionclass=`echo %{provides} | cut -d "_" -f2`
sed -i "s/SVN_NUM\s*=.*/SVN_NUM= $revision/g" ./Make.properties
sed -i "s/FC_VERSION\s*=.*/FC_VERSION= %{version}\.%{release}/g" ./Make.properties
sed -i 's/\.VER\./\./g' ./Make.properties
sed -i "s/VERSIONCLASS\s*=.*/VERSIONCLASS= $versionclass/g" ./Make.properties
make >/dev/null

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
PREFIX=$RPM_BUILD_ROOT make install-rpm  >/dev/null

%clean
rm -rf $RPM_BUILD_ROOT

%pre
/usr/sbin/groupadd squid >/dev/null 2>&1 || true
usr/sbin/useradd -d /var/spool/squid -g squid -s /sbin/nologin squid >/dev/null 2>&1 || true
#This for the situation squid user already exists 
#but home dir of squid  user had not created. In the 
#situation, the cron tasks in fc-squid-all will not 
#be executed, fix it.
sed -i 's/.*fcInfoCollector.sh.*//g' /var/spool/cron/root 
mkdir -p /var/spool/squid 
chown squid:squid /var/spool/squid
chmod 700 /var/spool/squid
chmod o+w /var/run

#increase the fd resource limit of named to 500000
sed -i '/^start\(\)/i\
ulimit -n 500000' /etc/init.d/named

/etc/init.d/named stop                  || true
/usr/bin/killall -9 billingd        >/dev/null 2>&1  || true
/usr/bin/killall -9 redirect        >/dev/null 2>&1  || true
/usr/bin/killall -9 refreshd        >/dev/null 2>&1  || true
/usr/bin/killall -9 moov_generator  >/dev/null 2>&1  || true
/usr/bin/killall -9 objsvr          >/dev/null 2>&1  || true
/usr/bin/killall -9 flexicache      >/dev/null 2>&1  || true
/usr/bin/killall -9 squid			>/dev/null 2>&1  || true
/usr/bin/killall -9 nginx           >/dev/null 2>&1  || true
/usr/bin/killall -9 ca_serverd      >/dev/null 2>&1  || true

[ -f /usr/local/squid/bin/lscs/sbin/nginx ] && rm -f /usr/local/squid/bin/lscs/sbin/nginx
[ -f /usr/local/squid/bin/flexicache ] && rm -f /usr/local/squid/bin/flexicache 
[ -f /var/named/chroot/etc/named.conf ] && cp -f /var/named/chroot/etc/named.conf  /var/named/chroot/etc/named.conf.fcrpmsave
rm -rf /usr/local/squid/sbin/modules/*

%post
cp /usr/local/squid/etc/IP2Location.h /usr/local/include/
[ "$(arch)" = "x86_64" ] && cp -f /usr/local/squid/etc/libIP2Location.so /lib64/ || cp -f /usr/local/squid/etc/libIP2Location.so /lib/

mkdir -p /var/log/chinacache 
chown squid:squid /var/log/chinacache
chmod 777 /var/log/chinacache
mkdir -p /data/refresh_db
chown squid:squid /data/refresh_db
mkdir -p /data/proclog/log/refreshd
chown squid:squid /data/proclog/log/refreshd
mkdir -p /data/proclog/log/squid/{archive,backup,billing,Microsoft,Receipt,note}
chown -R squid:squid /data/proclog/log/squid
chown squid:squid /usr/local/squid
chown squid:squid /usr/local/squid/etc
chown squid:squid /usr/local/squid/libexec
chown squid:squid /usr/local/squid/sbin
chown squid:squid /usr/local/squid/share
chown squid:squid /usr/local/squid/var
chmod 644 /etc/group 
mkdir -p /var/run/detectorig
chown root:root /var/run/detectorig
chmod o+rx /var/named /var/named/chroot /var/named/chroot/var /var/named/chroot/var/named
mkdir -p /data/proclog/log/flexicache
chown squid:squid /data/proclog/log/flexicache

chmod 755 /data/proclog/log/flexicache
test -e /usr/local/squid/etc/detectorig.conf||/bin/cp -f /usr/local/squid/etc/detectorig.conf.default /usr/local/squid/etc/detectorig.conf

test -e /usr/local/squid/etc/preloadmgr.conf||/bin/cp -f /usr/local/squid/etc/preloadmgr.conf.default /usr/local/squid/etc/preloadmgr.conf

#make sure that "service squid/fc XXX" could be excuted successfully
#[ -f /etc/init.d/squid ] && rm -f /etc/init.d/squid
#[ -f /etc/init.d/fc ] && rm -f /etc/init.d/fc
#[ -f /etc/init.d/flexicache ] && rm -f  /etc/init.d/flexicache
#cp -f /usr/local/squid/bin/squid_service /etc/init.d/squid
#ln -sf /etc/init.d/squid  /etc/init.d/fc
#ln -sf  /etc/init.d/squid /etc/init.d/flexicache

ln -sf /etc/init.d/fc /etc/init.d/squid
ln -sf /etc/init.d/fc /etc/init.d/flexicache
FC6ConfigFile=/usr/local/squid/etc/flexicache.conf
[ ! -f $FC6ConfigFile ] && mv $FC6ConfigFile.default $FC6ConfigFile
#[ -f /usr/local/squid/etc/nginx.conf.default ] && cp -f /usr/local/squid/etc/nginx.conf.default /usr/local/squid/bin/lscs/conf/nginx.conf

cp -f /var/named/chroot/etc/fc.named.conf /var/named/chroot/etc/named.conf

#we should rm the existing dst file before ln -sf a src file to that file
namedconf="/etc/named.conf"
[ -f "$namedconf" ] && rm -f $namedconf
ln -sf /var/named/chroot/etc/fc.named.conf /etc/named.conf
anyhost="/var/named/anyhost"
[ -f "$anyhost" ] || ln -sf /var/named/chroot/var/named/anyhost /var/named/anyhost
purgeGetoptssh="/usr/bin/purgeGetopts.sh"
[ -f "$purgeGetoptssh" ] &&  rm -f $purgeGetoptssh
ln -sf /usr/local/squid/bin/purgeGetopts.sh /usr/bin/purgeGetopts.sh

cd /tmp;tar -zxf monitor.tgz && test -d ./monitor.rpmsave && cd ./monitor.rpmsave; make >/dev/null 2>&1; make install >/dev/null 2>&1
#handle crontab
###crontab for user root###
/usr/bin/crontab -l > /tmp/root.tmp.$$
for Del in `cut -d " " -f7 /etc/cron.d/fc-root-all`
do
	grep -v "$Del" /tmp/root.tmp.$$ >/tmp/root.tmp.bak
	mv /tmp/root.tmp.bak /tmp/root.tmp.$$
done
sort /tmp/root.tmp.$$ > /tmp/rootSort.tmp.$$
uniq -f 5 /tmp/rootSort.tmp.$$ >/tmp/rootCrontab.tmp.$$
/usr/bin/crontab -u root /tmp/rootCrontab.tmp.$$
rm -f /tmp/root.tmp.$$
rm -f /tmp/rootSort.tmp.$$
rm -f /tmp/root.tmp.bak
rm -f /tmp/rootCrontab.tmp.$$
###crontab for user squid###
/usr/bin/crontab -u squid -l > /tmp/squid.tmp.$$
for Del in `cut -d " " -f7 /etc/cron.d/fc-squid-all`
do
	grep -v "$Del" /tmp/squid.tmp.$$ >/tmp/squid.tmp.bak
	mv /tmp/squid.tmp.bak /tmp/squid.tmp.$$
done
sed -i '/rotateAccess/'d /tmp/squid.tmp.$$
sort /tmp/squid.tmp.$$ > /tmp/squidSort.tmp.$$ 
uniq -f 5 /tmp/squidSort.tmp.$$ >/tmp/squidCrontab.tmp.$$
/usr/bin/crontab -u squid /tmp/squidCrontab.tmp.$$
rm -f /tmp/squid.tmp.$$
rm -f /tmp/squidSort.tmp.$$
rm -f /tmp/squid.tmp.bak
rm -f /tmp/squidCrontab.tmp.$$
###crontab for multi-squid mode###
sed -i '1,$s/.*flexicache.*//g' /var/spool/cron/root    
sed -i '/^$/d' /var/spool/cron/root
#handle end

#Does redirect.conf exist,if not,creat it
test -f /usr/local/squid/etc/redirect.conf \
|| touch /usr/local/squid/etc/redirect.conf;chown squid:squid /usr/local/squid/etc/redirect.conf
#create end

#Bug fix:s/external_dns external_dns 202.106.196.115/external_dns 202.106.196.115/g in
#"/usr/local/squid/etc/fcexternal.conf"
fcexternalconf="/usr/local/squid/etc/fcexternal.conf"
externaldnsip=`egrep "external_dns.*external_dns" $fcexternalconf 2>&1 | awk '{print $3}'`
[ ! -z $externaldnsip ]&&sed -i "s/external_dns.*/external_dns $externaldnsip/g" $fcexternalconf

chkconfig named on
/etc/init.d/named restart

versionclass=`echo %{provides} | cut -d "_" -f2`
if [ "${versionclass}" == 'cpis' ];then
    mv /etc/ChinaCache/app.d/fc.amr /etc/ChinaCache/app.d/cpisfc.amr
fi 
# check if exists refreshd.conf
if [ ! -e "/usr/local/squid/etc/refreshd.conf" ]
then
    /bin/cp /usr/local/squid/etc/refreshd.conf.default /usr/local/squid/etc/refreshd.conf
fi

. /etc/profile
if [ "$FCMODE" == "multiple" ];then
    /etc/init.d/fc start multiple	
else
    /etc/init.d/fc start single
fi
echo "FlexiCache Version: `/usr/local/squid/sbin/squid -v |grep Version |awk '{print \$4}'`" > /etc/motd 
#chkconfig squid on
#/etc/init.d/squid start&


%preun
#/etc/init.d/squid stop  	    	|| true
# back up nginx.conf
versionclass=`echo %{provides} | cut -d "_" -f2`
if [ "${versionclass}" == 'cpis' ];then
	mv /etc/ChinaCache/app.d/cpisfc.amr /etc/ChinaCache/app.d/fc.amr
fi
[ -f /usr/local/squid/etc/nginx.conf.default ] && rm -f /usr/local/squid/etc/nginx.conf.default
#[ -f /usr/local/squid/bin/lscs/conf/nginx.conf ] && cp -f /usr/local/squid/bin/lscs/conf/nginx.conf /usr/local/squid/etc/nginx.conf.default
true
#pkill -9 squid  || true

%postun
/usr/bin/killall -9 billingd        >/dev/null 2>&1 || true
/usr/bin/killall -9 refreshd        >/dev/null 2>&1 || true
/usr/bin/killall -9 moov_generator  >/dev/null 2>&1 || true
/usr/bin/killall -9 objsvr          >/dev/null 2>&1 || true
#test -L /var/named/anyhost && rm -f /var/named/anyhost
test -L /etc/named.conf && rm -f /etc/named.conf 
test -L /usr/bin/purgeGetopts.sh && rm -f /usr/bin/purgeGetopts.sh
test -L /etc/init.d/fc  && rm -f  /etc/init.d/fc
test -d /tmp/monitor.rpmsave && cd /tmp/monitor.rpmsave; make uninstall >/dev/null 2>&1
rm -rf  /tmp/monitor.rpmsave
test -f /var/named/chroot/etc/named.conf.fcrpmsave && cp -f /var/named/chroot/etc/named.conf.fcrpmsave /var/named/chroot/etc/named.conf
rm -f /var/named/chroot/etc/named.conf.fcrpmsave
rm -rf /usr/local/squid/bin/lscs/conf/nginx.conf
rm -rf /etc/init.d/squid &>/dev/null
rm -rf /etc/init.d/flexicache &> /dev/null

%files
%defattr(-,root,root) 
%attr(755,root,root)/etc/init.d/fc
%attr(600,root,root)/etc/cron.d/fc-root-all
%attr(644,root,root)/etc/cron.d/fc-squid-all
%attr(644,root,root)/etc/ChinaCache/app.d/fc.amr
%attr(755,root,root)/etc/profile.d/fc-path.sh
%attr(755,root,root)/usr/local/bin/robot.sh
%attr(755,root,root)/usr/local/detectorig/bin/digDetect
%attr(755,root,root)/usr/local/detectorig/bin/request
%attr(755,root,root)/usr/local/detectorig/sbin/detectorig
%attr(755,root,root)/usr/local/detectorig/sbin/named_reload
%attr(755,root,root)/usr/local/detectorig/sbin/digRun
%attr(755,root,root)/usr/local/detectorig/sbin/analyse
%attr(755,root,root)/usr/local/detectorig/module/mod_fc.so
%attr(755,root,root)/usr/local/detectorig/sbin/xping
%attr(644,root,root)/usr/local/squid/etc/detectorig.conf.default
%attr(644,root,root)/usr/local/squid/etc/preloader.conf.default
%attr(644,squid,squid)/usr/local/squid/etc/cachemgr.conf.default
%attr(644,root,root)/usr/local/squid/etc/domain.conf.default
%attr(644,squid,squid)/usr/local/squid/etc/mime.conf.default
%attr(644,root,root)/usr/local/squid/etc/refreshd.conf.default
%attr(644,root,root)/usr/local/squid/etc/count.conf.default
%attr(644,squid,squid)/usr/local/squid/etc/squid.conf.default
%attr(644,root,root)/usr/local/squid/etc/fcexternal.conf.default
%attr(644,squid,squid)/usr/local/squid/etc/flexicache.conf.default
%attr(644,root,root)/usr/local/squid/etc/IP2Location.h
%attr(644,root,root)/usr/local/squid/etc/libIP2Location.so
%attr(644,root,root)/usr/local/squid/etc/ipset
%attr(644,root,root)/usr/local/squid/etc/territory
%attr(644,squid,squid)/usr/local/squid/etc/cidrlist.conf
%attr(644,squid,squid)/usr/local/squid/etc/ipset_cn_v1.1
%attr(644,squid,squid)/usr/local/squid/etc/squid_cpis_all.conf
%attr(644,squid,squid)/usr/local/squid/etc/squid_cpis_video.conf
%attr(644,squid,squid)/usr/local/squid/etc/territory_cn_v1.1
%attr(755,squid,squid)/usr/local/squid/libexec/cachemgr.cgi
%attr(755,squid,squid)/usr/local/squid/libexec/diskd-daemon
%attr(755,squid,squid)/usr/local/squid/libexec/logfile-daemon
%attr(755,squid,squid)/usr/local/squid/libexec/unlinkd
%attr(755,squid,squid)/usr/local/squid/bin
%attr(755,squid,squid)/usr/local/squid/sbin/squid
%attr(755,squid,squid)/usr/local/squid/sbin/modules
%attr(-,squid,squid)/usr/local/squid/share/errors
%attr(-,squid,squid)/usr/local/squid/share/icons
%attr(-,squid,squid)/usr/local/squid/share/man
%attr(644,squid,squid)/usr/local/squid/share/mib.txt
%dir%attr(-,squid,squid)/usr/local/squid/var/logs
%attr(644,root,root)/usr/share/snmp/mibs/ChinaCache-MIB.txt
%attr(644,root,root)/var/named/chroot/var/named/anyhost
%attr(644,root,root)/etc/logrotate.d/squid
%attr(644,root,root)/var/named/chroot/etc/fc.named.conf
%attr(-,root,root)/tmp/monitor.tgz


