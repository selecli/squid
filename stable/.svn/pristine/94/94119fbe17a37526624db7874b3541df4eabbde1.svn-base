include ../../Make.properties

BIN=billingd

all: clean $(OBJS)
	$(EXD_LINK) -lpthread $(OBJS) -o $(BIN)

install-tar:
	cp -rf $(BIN) ../$(BIN_DIR)

install-rpm:
	cp -rf $(BIN) ../$(BIN_DIR)

clean:
	rm -rf $(OBJS) $(BIN)
	
uninstall:
	rm -rf /usr/local/squid/bin/$(BIN_DIR)/$(BIN)

%.o:    %.c
	$(EXD_GCC)  $< -o $@ -g
