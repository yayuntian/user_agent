LIBS=-lboost_regex

CFLAGS= -Wall -g -O3 -Werror
ifndef CXXFLAGS
	CXXFLAGS=-std=c++0x $(CFLAGS)
endif

KAFKA_LIBS = -lrdkafka

OBJS = userAgent.o operatingSystem.o bot.o browser.o
KAFKA_OBJS = kafkaConsumer.o
KAFKA_CAT = kafkacat.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: mafia kafkaConsumer kafkacat

mafia: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

kafkaConsumer: $(KAFKA_OBJS)
	$(CC) -o $@ $(KAFKA_OBJS) $(KAFKA_LIBS)

kafkacat: $(KAFKA_CAT)
	$(CC) -o $@ $(KAFKA_CAT) $(KAFKA_LIBS)
	
clean:
	rm -f *.o mafia kafkaConsumer
