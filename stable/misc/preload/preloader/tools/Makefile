GCC  = gcc -g -c -O2 -Wall
LINK = gcc -g -O2 -Wall

CSOURCE = $(wildcard *.c)
OBJS    = $(patsubst %.c,%.o,$(CSOURCE))
BIN     = preloader_client

all:$(OBJS)
	$(LINK) -o $(BIN) $(OBJS)

%.o:%.c
	$(GCC) $< -o $@

clean:
	$(RM) -rf *.o *~ $(BIN)
