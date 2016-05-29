GCC		= gcc -Wall -c -O2
G++		= g++ -Wall -c -O2
LINK	= gcc -Wall -O2

INCLUDE	=
LIB		=
BIN		= objsvr

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