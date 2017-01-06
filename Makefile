LIBS=-lboost_regex
ifndef CXXFLAGS
	CXXFLAGS=-std=c++0x -Wall -fPIC -g -O3 -Werror
endif

KAFKA_LIBS = -lrdkafka

OBJS = userAgent.o operatingSystem.o bot.o browser.o
KAFKA_OBJS = kafkaConsumer.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

all: mafia

mafia: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

kafkaConsumer: $(KAFKA_OBJS)
	$(CXX) -o $@ $(KAFKA_OBJS) $(KAFKA_LIBS)

clean:
	rm -f *.o mafia kafkaConsumer
