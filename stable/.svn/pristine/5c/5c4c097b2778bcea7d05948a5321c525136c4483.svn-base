include ../../Make.properties

INCLUDE	= -I../include -I/usr/include/libxml2/
LIB		= -lxml2 -lpthread
BIN		= refreshd
SUB_BINS = refresh_cli check_db hash_db
#CFLAGS	+= -D__THREAD_SAFE__ -D_FILE_OFFSET_BITS=64 #-DDEBUG
CFLAGS	+=  -D_FILE_OFFSET_BITS=64 #-DDEBUG

all: clean $(OBJS)
	$(EXD_LINK) -o $(BIN) $(OBJS) $(LIB) $(INCLUDE) $(LIBRARY) $(CFLAGS)
	make -C tools

install-tar:
	cd tools;make install;
	cp -f $(BIN) $(SUB_BINS) ../$(BIN_DIR)
	cp -f tools/refreshd.conf ../$(ETC_DIR)/refreshd.conf.default
install-rpm:
	cd tools;make install;
	cp -f $(BIN) $(SUB_BINS) ../$(BIN_DIR)
	cp -f tools/refreshd.conf ../$(ETC_DIR)/refreshd.conf.default

clean:
	rm -rf $(BIN) $(OBJS)
	make -C tools clean

%.o:	%.c
	$(EXD_GCC) $< -rdynamic -o $@ $(INCLUDE) $(CFLAGS)
