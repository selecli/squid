include ../../Make.properties

all: 

install-tar:
	cp -rf *.sh ../../$(SQUID_DIR)/$(BIN_DIR)
	cp -rf *.pl ../../$(SQUID_DIR)/$(BIN_DIR)
	cp -rf *.py ../../$(SQUID_DIR)/$(BIN_DIR)
	cp -rf *.pm ../../$(SQUID_DIR)/$(BIN_DIR)
	chmod a+x squidlog
	cp -rf squidlog ../../$(SQUID_DIR)/$(BIN_DIR)
	cp -rf lscsConfig ../../$(SQUID_DIR)/$(BIN_DIR)
	cp -rf dnsSwitch/ ../../$(SQUID_DIR)/$(BIN_DIR)
	cp territory_cn_v1.1 ../../$(SQUID_DIR)/$(ETC_DIR)
	cp ipset_cn_v1.1 ../../$(SQUID_DIR)/$(ETC_DIR)
	cp fcexternal.conf ../../$(SQUID_DIR)/$(ETC_DIR)
	cp ip2location/IP2Location.h ../../$(SQUID_DIR)/$(ETC_DIR)
	test "$(PLATFORM)" = "x86_64" && cp ip2location/libIP2Location.so.64 ../../$(SQUID_DIR)/$(ETC_DIR)/libIP2Location.so || cp ip2location/libIP2Location.so.32 ../../$(SQUID_DIR)/$(ETC_DIR)/libIP2Location.so
	cp mydbset/* ../../$(SQUID_DIR)/$(ETC_DIR)

install-rpm:
	cp -rf *.sh $(DESTDIR)/usr/local/squid/bin
	cp -rf *.pl $(DESTDIR)/usr/local/squid/bin 
	cp -rf *.py $(DESTDIR)/usr/local/squid/bin 
	cp -rf *.pm $(DESTDIR)/usr/local/squid/bin 
	cp -rf gbtoall $(DESTDIR)/usr/local/squid/bin 
	cp -rf lscsConfig $(DESTDIR)/usr/local/squid/bin 
	cp -rf dnsSwitch/ $(DESTDIR)/usr/local/squid/bin
	test $(DESTDIR)/usr/local/squid/bin/dnsSwitch/.svn && rm -rf $(DESTDIR)/usr/local/squid/bin/dnsSwitch/.svn
	cp territory_cn_v1.1 $(DESTDIR)/usr/local/squid/etc
	cp ipset_cn_v1.1  $(DESTDIR)/usr/local/squid/etc
	cp squid_cpis_video.conf $(DESTDIR)/usr/local/squid/etc
	cp squid_cpis_all.conf  $(DESTDIR)/usr/local/squid/etc
	cp cidrlist.conf $(DESTDIR)/usr/local/squid/etc
	cp -f fcexternal.conf $(DESTDIR)/usr/local/squid/etc/fcexternal.conf.default
	cp -f ip2location/IP2Location.h $(DESTDIR)/usr/local/squid/etc
	test "$(PLATFORM)" = "x86_64" && cp -f ip2location/libIP2Location.so.64 $(DESTDIR)/usr/local/squid/etc/libIP2Location.so || cp -f ip2location/libIP2Location.so.32 $(DESTDIR)/usr/local/squid/etc/libIP2Location.so
	cp -rf mydbset/* $(DESTDIR)/usr/local/squid/etc
clean:
	
uninstall:
	rm -rf /usr/local/squid/bin/$(BIN)/miniftt
