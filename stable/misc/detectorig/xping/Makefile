GCC=gcc -c -Wall -O2 -g
LINK=gcc -Wall -O2 -g


BIN=xping
CSOURCE=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(CSOURCE))

all:$(OBJS)
	$(LINK) -o $(BIN) $(OBJS)

%.o:%.c
	$(GCC) $< -o $@

clean:
	$(RM) -rf *.o *~ $(BIN)

