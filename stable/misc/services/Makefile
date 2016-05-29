include ../../Make.properties

all:
install-tar:
	-test `uname -m` = "i686" && cp -f squidlog maint/squidlog
	-test `uname -m` = "i386" && cp -f squidlog maint/squidlog 
	-test `uname -m` = "x86_64" && cp -f squidlog_x86_64 maint/squidlog
	-test "$(VERSIONCLASS)" = "std" && cp -f Step3.setup.sh ../../$(INSTALL_DIR)/Step3.setup.sh 
	-test "$(VERSIONCLASS)" = "cpis"&& cp -f Step3.setup_GAD.sh ../../$(INSTALL_DIR)/Step3.setup.sh 
	echo $(FC_NAME) > ReleaseTag.txt
	cp -rf ChinaCache-MIB.txt InstallSquidsetup.sh initiatingSquid.sh ../../$(INSTALL_DIR)
	cp -rf isnewswap monitorReInstall.sh robotInstall.sh rotate.squid ../../$(INSTALL_DIR)
	cp -rf maint ssr root_crontab ReleaseTag.txt  named.conf.squid  robotUninstall.sh  ../../$(INSTALL_DIR)
	cp -rf Step1.prepare.sh Step5.tunningServices.sh UninstallSquidsetup.sh ../../$(INSTALL_DIR)
	cp -rf monitor.sh robot.sh squid_crontab ../../$(INSTALL_DIR)
	cp -f squid_service_multisquid ReadBackwards.pm squid_service flexicache_service fc_service functions fcClient fcCheck ../../$(INSTALL_DIR)/usr/local/squid/bin

install-rpm:
	mkdir -p $(DESTDIR)/etc
	mkdir -p $(DESTDIR)/etc/init.d
	mkdir -p $(DESTDIR)/etc/cron.d
	mkdir -p $(DESTDIR)/etc/profile.d
	mkdir -p $(DESTDIR)/etc/ChinaCache/app.d
	mkdir -p ${DESTDIR}/var/named/chroot/var/named
	mkdir -p ${DESTDIR}/var/named/chroot/etc
	mkdir -p $(DESTDIR)/usr/local/bin
	mkdir -p $(DESTDIR)/usr/share/snmp/mibs
	mkdir -p $(DESTDIR)/usr/local/squid/bin 
	mkdir -p $(DESTDIR)/etc/logrotate.d 
	cp -f ./fc_service  $(DESTDIR)/etc/init.d/fc
	cp -f ./functions  $(DESTDIR)/usr/local/squid/bin
	cp -f ./ReadBackwards.pm  $(DESTDIR)/usr/local/squid/bin
	cp -f ./fcClient $(DESTDIR)/usr/local/squid/bin
	cp -f ./fcCheck $(DESTDIR)/usr/local/squid/bin 
	cp -f ./squid_service $(DESTDIR)/usr/local/squid/bin
	cp -f ./squid_service_multisquid $(DESTDIR)/usr/local/squid/bin
	cp -f ./flexicache_service $(DESTDIR)/usr/local/squid/bin
	-test "$(VERSIONCLASS)" = "std" && cp -f ./fc.amr $(DESTDIR)/etc/ChinaCache/app.d/fc.amr
	-test "$(VERSIONCLASS)" = "cpis" && cp -f ./cpisfc.amr $(DESTDIR)/etc/ChinaCache/app.d/fc.amr
	cp -f ./named.conf.squid  $(DESTDIR)/var/named/chroot/etc/fc.named.conf
	cp -f ./anyhost $(DESTDIR)/var/named/chroot/var/named/anyhost
	cp -f ./monitor.sh $(DESTDIR)/usr/local/squid/bin/monitor.sh
	cp -f ./robot.sh $(DESTDIR)/usr/local/bin/robot.sh
	cp -f ./ChinaCache-MIB.txt $(DESTDIR)/usr/share/snmp/mibs/ChinaCache-MIB.txt
	-test "$(VERSIONCLASS)" = "std" && cp -f ./root_crontab   $(DESTDIR)/etc/cron.d/fc-root-all
	-test "$(VERSIONCLASS)" = "cpis" && cp -f ./cpis_root_crontab  $(DESTDIR)/etc/cron.d/fc-root-all
	-test "$(VERSIONCLASS)" = "std" && cp -f ./squid_crontab  $(DESTDIR)/etc/cron.d/fc-squid-all
	-test "$(VERSIONCLASS)" = "cpis" && touch  $(DESTDIR)/etc/cron.d/fc-squid-all
	cp -f ./fc-path.sh  $(DESTDIR)/etc/profile.d/fc-path.sh
	-test "$(PLATFORM)" = "i686" && cp -f ./squidlog $(DESTDIR)/usr/local/squid/bin/squidlog
	-test "$(PLATFORM)" = "i386" && cp -f ./squidlog $(DESTDIR)/usr/local/squid/bin/squidlog
	-test "$(PLATFORM)" = "x86_64" && cp -f ./squidlog_x86_64 $(DESTDIR)/usr/local/squid/bin/squidlog
	cp -f ./gatherinfo $(DESTDIR)/usr/local/squid/bin/gatherinfo
	cp -f ./purgeGetopts.sh $(DESTDIR)/usr/local/squid/bin/purgeGetopts.sh
	cp -f ./purgebdb.sh $(DESTDIR)/usr/local/squid/bin/purgebdb.sh
	cp -f ./rotate.squid $(DESTDIR)/etc/logrotate.d/squid
	cp -f ./syn_cron.sh $(DESTDIR)/usr/local/squid/bin/syn_cron.sh
clean:
