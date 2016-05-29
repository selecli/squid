GCC     = gcc
LIBS    = -l pthread -l ssl -l xml2
INCLUDE = -I /usr/include/libxml2
CFLAGS  = -c -g -O2 -Wall -D _GNU_SOURCE

CSOURCE = $(wildcard *.c)
OBJS    = $(patsubst %.c,%.o,$(CSOURCE))
ALLOBJS = *.o
BIN     = preloader

all:$(OBJS)
	$(GCC) $(INCLUDE) $(LIBS) $(ALLOBJS) -o $(BIN)

%.o:%.c
	$(GCC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	@rm -f *.o *~ $(BIN)
