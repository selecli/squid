include ../../Make.properties

INCLUDE	=  
LIB		= -lpthread
BIN		= ca_serverd
#CFLAGS	+= -D__THREAD_SAFE__ -D_FILE_OFFSET_BITS=64 #-DDEBUG
CFLAGS	+=  -D_FILE_OFFSET_BITS=64 #-DDEBUG

GCC     = gcc -Wall -g -rdynamic -c 
LINK    = gcc -Wall 

SOURCE  = $(wildcard *.c)

OBJS    = $(SOURCE:%.c=%.o)

all: clean $(OBJS)
	$(LINK) -o $(BIN) $(OBJS) $(LIB)

install-tar:
	cp -f $(BIN) ../$(BIN_DIR)

install-rpm:
	cp -f $(BIN) ../$(BIN_DIR)

clean:
	rm -rf $(BIN) $(OBJS)

%.o: %.c
	$(GCC) $(INCLUDE) $< -o $@
