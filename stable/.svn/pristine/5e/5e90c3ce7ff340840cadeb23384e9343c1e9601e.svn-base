GCC	= gcc -Wall -c -O2 -fpic -fPIC --shared
G++	= g++ -Wall -c -O2 -fpic -fPIC --shared
LINK	= gcc -Wall -O2 -shared

INCLUDE	= -I../misc
LIB	=
BIN	= libmisc.so

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
