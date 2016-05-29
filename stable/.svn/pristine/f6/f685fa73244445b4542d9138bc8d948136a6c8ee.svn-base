GCC	= gcc -Wall -c -O2
G++	= g++ -Wall -c -O2
LINK	= gcc -Wall -O2

INCLUDE	= -I../misc -I../detect/src
LIB	= ../misc/misc.c ../misc/child_mgr.c ../detect/src/write_rcms_log.c
BIN	= digRun

CSOURCE		= $(wildcard *.c)
C++SOURCE	= $(wildcard *.cpp)

OBJS	= $(patsubst %.c,%.o,$(CSOURCE)) $(patsubst %.cpp,%.o,$(C++SOURCE))


all:	$(OBJS)
	$(LINK) -o $(BIN) $(OBJS) $(LIB)

%.o: %.c
	$(GCC) $(INCLUDE) $< -o $@

%.o: %.cpp
	$(G++) $(INCLUDE) $< -o $@

clean:
	rm -f *.o $(BIN)
