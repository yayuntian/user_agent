FLAGS = -Wall -g -O3 -Werror
FLAGS += -DPERF
FLAGS += -DLRU_CACHE
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

# unit test

UA_OBJS = test_ua.o ipLocator.o wrapper.o \
                    userAgent.o operatingSystem.o bot.o browser.o
ua_test: $(UA_OBJS)
	$(CXX) -o $@ $(UA_OBJS) $(CXXLIBS)

IP_OBJS = test_ip.o ipLocator.o wrapper.o \
                    userAgent.o operatingSystem.o bot.o browser.o
ip_test: $(IP_OBJS)
	$(CXX) -o $@ $(IP_OBJS) $(CXXLIBS)


.PHONY: clean
clean:
	rm -f *.o $(TARGET) json ua_test ip_test
