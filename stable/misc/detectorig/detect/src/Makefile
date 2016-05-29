GCC=gcc -c -O2 -Wall -g
LINK=gcc -O2 -Wall -g
LIBS=-lssl

CSOURCE=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(CSOURCE))
BIN=detectorig

all:$(OBJS)
	$(LINK) -o $(BIN) $(OBJS) $(LIBS)
	cp -f $(BIN) ..

%.o:%.c
	$(GCC) $< -o $@

clean:
	$(RM) -rf *.o *~ $(BIN)
