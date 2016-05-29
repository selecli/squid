#!/bin/sh
include ./Make.properties

SQUID_CONF_FLAGS := --enable-cc-framework --enable-cc-multi_core --enable-kill-parent-hack --enable-large-cache-files --with-large-files --with-maxfd=344800 --enable-snmp --enable-internal-dns --disable-ident-lookups --enable-carp --enable-async-io=160 --enable-storeio=ufs,aufs,diskd,null,coss --enable-epoll --enable-linux-netfilter --enable-linux-tproxy --enable-stacktraces --enable-forward-log --enable-referer-log --enable-useragent-log --enable-delay-pools --enable-follow-x-forwarded-for --enable-forw-via-db --enable-ssl --enable-default-err-language=Simplify_Chinese --enable-err-languages="Simplify_Chinese English" --without-system-md5 --with-openssl=/usr/local/openssl

ifeq ($(PLATFORM),x86_64)
	SQUID_CONF_FLAGS += --with-build-environment=POSIX_V6_LP64_OFF64
	resovl_path=/usr/lib64
endif

all:
	$(SQUID_REPL_VER);
	export CFLAGS=-O2;cd squid; ./configure $(SQUID_CONF_FLAGS)
	make -C squid || exit 1
	export CFLAGS=-O2;make -C modules || exit 1
	export CFLAGS=-O2;make -C misc || exit 1

debug:
	$(SQUID_REPL_VER);
	export CFLAGS=-g ;cd squid; ./configure $(SQUID_CONF_FLAGS)
	export CFLAGS=-g ;make -C squid || exit 1
	export CFLAGS=-g;make -C modules || exit 1
	export CFLAGS=-g;make -C misc || exit 1

install-rpm:
	make -C squid install DESTDIR=$(PREFIX)
	make -C modules install-rpm DESTDIR=$(PREFIX)/usr/local/squid/sbin/
	make -C misc install-rpm DESTDIR=$(PREFIX)
	rm -f $(PREFIX)/usr/local/squid/etc/squid.conf
	rm -f $(PREFIX)/usr/local/squid/etc/mime.conf
	mv $(PREFIX)/usr/local/squid/etc/cachemgr.conf $(PREFIX)/usr/local/squid/etc/cachemgr.conf.default
install-tar:
	rm -rf $(INSTALL_DIR);mkdir $(INSTALL_DIR);
	make -C squid install DESTDIR=/`pwd`/$(INSTALL_DIR)
	rm -rf $(SQUID_DIR)/etc/squid.conf
	make -C modules install-tar
	make -C misc install-tar
	mv $(INSTALL_DIR) $(FC_NAME)
	-test "$(VERSIONCLASS)" = "std" && tar cvzf $(FC_NAME).tgz ./$(FC_NAME) && mv $(FC_NAME).tgz ..
	-test "$(VERSIONCLASS)" = "cpis" && mv $(FC_NAME) $(FC_NAME)_CPIS\
		&& tar cvzf $(FC_NAME)_CPIS.tgz ./$(FC_NAME)_CPIS && mv $(FC_NAME)_CPIS.tgz ..
rpm: 
	rpmbuild --buildroot /tmp/fc-build-${USER} -bb fc.spec
tar:	
	make
	make install-tar	
clean:
	make -C squid clean;
	make -C modules clean;
	make -C misc clean;
