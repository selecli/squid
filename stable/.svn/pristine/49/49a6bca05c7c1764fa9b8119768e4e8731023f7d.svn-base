include ../Make.properties

NGINX_CONFIG_FLAGS := --add-module=./lscs_module --with-cc-opt="-g -O0" --prefix=/usr/local/squid/bin/lscs
lscs = nginx-0.8.55

EXDSTAR = billingd refreshd redirect logdealer preload h264_streamd services flexicache h264_streamd_for_pplive count ca_server
EXDS = $(EXDSTAR) detectorig 

all: clean
	export CFLAGS=-O2; cd $(lscs); ./configure $(NGINX_CONFIG_FLAGS)
	make -C $(lscs) || exit 1
	for i in $(EXDS); do make -C $$i || exit 1; done

install-rpm:
	rm -rf $(BIN_DIR) $(ETC_DIR);mkdir $(BIN_DIR);mkdir $(ETC_DIR)
	for i in $(EXDS); do cd $$i;make install-rpm DESTDIR=${DESTDIR};cd ..; done
	#-test -d ./$(lscs) && ls -alR ./$(lscs) | grep "\.svn:" | cut -d ":" -f1 | xargs rm -rf
	make -C $(lscs) install  DESTDIR=$(DESTDIR)
	cp -rf $(BIN_DIR)/*  $(DESTDIR)/usr/local/squid/bin
	cp -rf $(ETC_DIR)/*  $(DESTDIR)/usr/local/squid/etc
	rm -rf $(BIN_DIR) $(ETC_DIR)
	cp -rf ./monitor ./monitor.rpmsave 
	-test -d ./monitor.rpmsave/.svn && ls -laR ./monitor.rpmsave|grep "\.svn:" | cut -d ":" -f 1 | xargs rm -rf 
	tar -czf monitor.tgz ./monitor.rpmsave
	mkdir -p $(DESTDIR)/tmp
	cp -f monitor.tgz $(DESTDIR)/tmp/monitor.tgz 
	rm -f monitor.tgz
	rm -rf monitor.rpmsave

install-tar:
	rm -rf $(BIN_DIR) $(ETC_DIR); mkdir $(BIN_DIR);mkdir $(ETC_DIR)
	for i in $(EXDSTAR); do cd $$i;make install-tar;cd ..; done
	make -C $(lscs) install DESTDIR=../../$(INSTALL_DIR)/
	cp -rf $(BIN_DIR)  ../$(SQUID_DIR)/;
	cp -rf $(ETC_DIR)  ../$(SQUID_DIR)/;
	rm -rf $(BIN_DIR) $(ETC_DIR);
	cd ..
	cp -rf services/ detectorig/ monitor/ ../$(INSTALL_DIR);
	cp -rf Makefile.package ../$(INSTALL_DIR)/Makefile;

clean:
	for i in $(EXDS); do cd $$i;make clean;cd ..; done
	-test -f ./$(lscs)/Makefile && make -C $(lscs) clean
	rm -rf $(BIN_DIR) $(ETC_DIR);
