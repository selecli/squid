include ../../Make.properties

DET_PATH = "/usr/local/detectorig"
DEST_DIR=../../$(INSTALL_DIR)/detectorig

all: 
	make -C dig
	make -C xping
	make -C detect
	make -C request
	make -C dig_detect
	mkdir -p analyse/build; cd analyse/build; cmake ..; make;

install-tar:
	rm -rf $(DEST_DIR);mkdir $(DEST_DIR)
	rm -rf $(DEST_DIR)/bin;mkdir $(DEST_DIR)/bin
	rm -rf $(DEST_DIR)/sbin;mkdir $(DEST_DIR)/sbin
	rm -rf $(DEST_DIR)/module;mkdir $(DEST_DIR)/module
	rm -rf $(DEST_DIR)/etc;mkdir $(DEST_DIR)/etc
	cp -f dig/digRun $(DEST_DIR)/sbin
	cp -f xping/xping $(DEST_DIR)/sbin
	cp -f analyse/build/sbin/analyse $(DEST_DIR)/sbin
	cp -f detect/detectorig $(DEST_DIR)/sbin
	cp -f request/request $(DEST_DIR)/bin
	cp -f dig_detect/digDetect $(DEST_DIR)/bin
	cp -f script/named_reload $(DEST_DIR)/sbin
	cp -f analyse/build/module/mod_fc.so $(DEST_DIR)/module
	cp -f analyse/build/module/mod_tta.so $(DEST_DIR)/module
	cp -f domain.conf $(DEST_DIR)/etc
	cp -f detect/etc/detectorig.conf $(DEST_DIR)/etc
	cp -f detectorig_install.sh $(DEST_DIR)

install-rpm:
	@test -d $(DESTDIR)/$(DET_PATH)/bin||mkdir -p $(DESTDIR)/$(DET_PATH)/bin
	@/bin/cp -f request/request $(DESTDIR)/$(DET_PATH)/bin
	@/bin/cp -f dig_detect/digDetect $(DESTDIR)/$(DET_PATH)/bin
	@test -d $(DESTDIR)/$(DET_PATH)/sbin||mkdir -p $(DESTDIR)/$(DET_PATH)/sbin
	@/bin/cp -f detect/detectorig $(DESTDIR)/$(DET_PATH)/sbin
	@/bin/cp -f dig/digRun $(DESTDIR)/$(DET_PATH)/sbin
	@/bin/cp -f analyse/build/sbin/analyse $(DESTDIR)/$(DET_PATH)/sbin
	@test -d $(DESTDIR)/$(DET_PATH)/module || mkdir -p $(DESTDIR)/$(DET_PATH)/module
	@/bin/cp -f analyse/build/module/mod_fc.so $(DESTDIR)/$(DET_PATH)/module
	@test -d $(DESTDIR)/$(DET_PATH)/route_result||mkdir -p $(DESTDIR)/$(DET_PATH)/route_result
	@/bin/cp -f xping/xping $(DESTDIR)/$(DET_PATH)/sbin
	@/bin/cp -f script/named_reload $(DESTDIR)/$(DET_PATH)/sbin
	@test -d $(DESTDIR)/usr/local/squid/etc||mkdir -p $(DESTDIR)/usr/local/squid/etc
	@/bin/cp -f domain.conf $(DESTDIR)/usr/local/squid/etc/domain.conf.default
	@/bin/cp -f detect/etc/detectorig.conf $(DESTDIR)/usr/local/squid/etc/detectorig.conf.default

clean:
	make -C misc clean
	make -C detect clean
	make -C dig clean
	make -C dig_detect clean
	make -C request	clean
	make -C xping clean
	rm -rf analyse/build

