LIBS=-lboost_regex
ifndef CXXFLAGS
	CXXFLAGS=-std=c++0x -Wall -fPIC -g -O3 -Werror
endif

OBJS = ua.o os.o bot.o

%.o: %.cpp
	    $(CXX) $(CXXFLAGS) -c $<

all: mafia

mafia: $(OBJS)
	    $(CXX) -o $@ $(OBJS) $(LIBS)

clean:
	    rm -f *.o mafia
