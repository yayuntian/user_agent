LIBS=-lboost_regex

CFLAGS= -Wall -g -O0 -Werror
ifndef CXXFLAGS
	CXXFLAGS=-std=c++0x $(CFLAGS)
endif

KAFKA_LIBS = -lrdkafka

OBJS = userAgent.o operatingSystem.o bot.o browser.o
KAFKA_OBJS = kafkaExample.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: mafia kafkaExample

mafia: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

kafkaExample: $(KAFKA_OBJS)
	$(CC) -o $@ $(KAFKA_OBJS) $(KAFKA_LIBS)
	
clean:
	rm -f *.o mafia kafkaExample
