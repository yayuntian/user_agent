FLAGS= -Wall -g -O0 -Werror
CXXFLAGS += -std=c++0x $(FLAGS)
CFLAGS += -std=c99 -msse4.2 $(FLAGS)

KAFKA_LIBS = -lrdkafka
CXXLIBS = -lboost_regex

OBJS = userAgent.o operatingSystem.o bot.o browser.o kafkaConsumer.o

JSON_OBJS = main.o extractor.o

TARGET = mafia

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(CXXLIBS) $(KAFKA_LIBS)

json: $(JSON_OBJS)
	$(CC) -o $@ $(JSON_OBJS) $(KAFKA_LIBS)

.PHONY: clean
clean:
	rm -f *.o $(TARGET)
