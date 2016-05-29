include ../../Make.properties

INCLUDE	=
#LIB = -L/usr/lib -lpthread 
LIB =  -lpthread 
CSOURCE	= $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(CSOURCE))
BIN = moov_generator_for_pplive
CFLAGS += -D__THREAD_SAFE__

all: clean $(OBJS)
	$(EXD_LINK) -o $(BIN) $(OBJS) $(LIB)
#	$(LINK) -o $(BIN) $(OBJS) $(LIB)

clean:
	rm -rf $(OBJS) $(BIN)

test: test.o
	$(EXD_LINK) -o test test.o log.o
#	$(LINK) -o test test.o log.o

install-tar: all
	cp -f moov_generator_for_pplive ../$(BIN_DIR)
	#/usr/local/squid/bin/moov_generator -D

install-rpm: all
	cp -f moov_generator_for_pplive ../$(BIN_DIR)

uninstall:
	rm -rf /usr/local/include/cc*.h
	rm -rf /usr/local/lib/libcc*.so

%.o:    %.c
	$(EXD_GCC) $(INCLUDE) $< -o $@
#	$(GCC) $(INCLUDE) $< -o $@
