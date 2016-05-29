GCC	= gcc -Wall -c -O2
G++	= g++ -Wall -c -O2
LINK	= gcc -Wall -O2

INCLUDE	= -I../misc 
BIN	= request

CSOURCE		= $(wildcard *.c)
C++SOURCE	= $(wildcard *.cpp)

OBJS	= $(patsubst %.c,%.o,$(CSOURCE)) $(patsubst %.cpp,%.o,$(C++SOURCE))


all:	$(OBJS)
	$(LINK) -o $(BIN) $(OBJS) 

%.o: %.c
	$(GCC) $(INCLUDE) $< -o $@

%.o: %.cpp
	$(G++) $(INCLUDE) $< -o $@

clean:
	rm -f *.o $(BIN)
