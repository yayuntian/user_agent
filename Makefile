FLAGS = -Wall -g -O0 -Werror
FLAGS += -DPERF
#FLAGS += -DLRU_CACHE
CXXFLAGS += -std=c++0x $(FLAGS)
CFLAGS += -std=c99 -msse4.2 $(FLAGS)

KAFKA_LIBS = -lrdkafka
CXXLIBS = -lboost_regex

OBJS = userAgent.o operatingSystem.o bot.o \
	   browser.o kafkaConsumer.o    \
       main.o extractor.o ipLocator.o wrapper.o

TARGET = mafia

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(CXXLIBS) $(KAFKA_LIBS)

JSON_OBJS = main.o extractor.o ipLocator.o wrapper.o \
    userAgent.o operatingSystem.o bot.o browser.o
json: $(JSON_OBJS)
	$(CXX) -o $@ $(JSON_OBJS) $(CXXLIBS) $(KAFKA_LIBS)

UA_OBJS = test_ua.o userAgent.o operatingSystem.o bot.o browser.o
ua: $(UA_OBJS)
	$(CXX) -o $@ $(UA_OBJS) $(CXXLIBS)

IP_OBJS = test_ip.o ipLocator.o wrapper.o \
userAgent.o operatingSystem.o bot.o browser.o
ip: $(IP_OBJS)
	$(CXX) -o $@ $(IP_OBJS) $(CXXLIBS)


.PHONY: clean
clean:
	rm -f *.o $(TARGET) json ua ip
