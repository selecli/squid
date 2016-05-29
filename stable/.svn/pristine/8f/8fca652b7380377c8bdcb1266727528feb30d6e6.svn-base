SUBDIRS = src tools
BIN     = preloader preloader_client

all:
	@for i in $(SUBDIRS); do make -C $$i || exit 1; done

clean:
	@for i in $(SUBDIRS); do make -C $$i clean || exit 1; done
	@rm -f $(BIN)
