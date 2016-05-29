include ../../Make.properties

GCC 	= gcc -Wall -c -O2
LINK 	= gcc -Wall -O2
#GCC 	= gcc -Wall -c -ggdb
#LINK 	= gcc -Wall -ggdb

INCLUDE	=
LIB		=
BIN		= count

SOURCE	= $(wildcard *.c)

OBJS	= $(SOURCE:%.c=%.o)

all:	$(OBJS)
	$(LINK) -o $(BIN) $(OBJS) $(LIB)
	#$(EXD_LINK) -o $(BIN) $(OBJS) $(LIB)

%.o: %.c
	$(GCC) $(INCLUDE) $< -o $@
	#$(EXD_GCC) $(INCLUDE) $< -o $@

install-tar:
	cp -rf $(BIN) ../$(BIN_DIR)
	cp -f ./count.conf.default ../$(ETC_DIR)

install-rpm:
	cp -rf $(BIN) ../$(BIN_DIR)
	cp -f ./count.conf.default ../$(ETC_DIR)

clean:
	rm -rf $(OBJS) $(BIN)

uninstall:
	rm -rf /usr/local/squid/bin/$(BIN_DIR)/$(BIN)
