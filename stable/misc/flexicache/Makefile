include ../../Make.properties

BIN = flexicache

all: clean $(OBJS)
	$(EXD_LINK) $(OBJS) -o $(BIN)

install-tar:
	cp -rf $(BIN) ../$(BIN_DIR)
	cp flexicache.conf.default ../$(ETC_DIR)

install-rpm:
	cp -rf $(BIN) ../$(BIN_DIR)
	cp flexicache.conf.default ../$(ETC_DIR)

clean:
	rm -rf $(OBJS) $(BIN)

uninstall:
	rm -rf /usr/local/squid/bin/$(BIN_DIR)/$(BIN)

%.o:    %.c 
	$(EXD_GCC)  $< -o $@
