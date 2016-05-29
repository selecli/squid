include ../../Make.properties
all:
	make -C preloader
	make -C preloadmgr
	make -C objectserver
	make -C flexiget

install-tar:	all
	cp -fr preloader/preloader	preloadmgr/preloadmgr preloadmgr/mkdir.sh  objectserver/objsvr  flexiget/flexiget ../$(BIN_DIR)

install-rpm:
	cp -fr preloader/src/preloader preloadmgr/preloadmgr preloadmgr/mkdir.sh  objectserver/objsvr  flexiget/flexiget ../$(BIN_DIR)
	@/bin/cp -f preloader/conf/preloader.conf ../$(ETC_DIR)/preloader.conf.default

clean:
	make clean -C preloader
	make clean -C preloadmgr
	make clean -C objectserver
	make clean -C flexiget
	rm -fr flexiget/flexiget
	rm -fr objectserver/objsvr
	rm -fr preloader/src/preloader
	rm -fr preloadmgr/preloadmgr
