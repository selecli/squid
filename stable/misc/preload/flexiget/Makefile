include ../../../Make.properties
GCC		= gcc -Wall -c -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE $(CFLAGS)
G++		= g++ -Wall -c -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE $(CFLAGS)
LINK	= gcc -Wall -O2 $(CFLAGS)

INCLUDE	=
LIB		= -lpthread
BIN		= flexiget

CSOURCE		= $(wildcard *.c)
C++SOURCE	= $(wildcard *.cpp)

OBJS		= $(patsubst %.c,%.o,$(CSOURCE)) $(patsubst %.cpp,%.o,$(C++SOURCE))


all:	$(OBJS)
	$(LINK) -o $(BIN) $(OBJS) $(LIB)

%.o: %.c
	$(GCC) $(INCLUDE) $< -o $@

%.o: %.cpp
	$(G++) $(INCLUDE) $< -o $@

clean:
	rm -rf $(OBJS) $(BIN)
