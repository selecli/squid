GCC		= gcc -Wall -Werror -c -g -O2
LINK	= gcc -Wall -Werror -g -O2 
INCLUDE	=
LIB = -L/usr/lib 
CSOURCE	= $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(CSOURCE))
BIN = moov_transfer

all: clean $(OBJS)
	$(LINK) -o $(BIN) $(OBJS) $(LIB)

clean:
	rm -rf $(OBJS) $(BIN)

test: test.o
	$(LINK) -o test test.o log.o

install: all
	cp -f moov_generator /usr/local/squid/bin/moov_generator
	/usr/local/squid/bin/moov_generator -D
uninstall:
	rm -rf /usr/local/include/cc*.h
	rm -rf /usr/local/lib/libcc*.so

%.o:    %.c
	$(GCC) $(INCLUDE) $< -o $@

