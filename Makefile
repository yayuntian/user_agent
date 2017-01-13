FLAGS = -Wall -g -O3 -Werror
FLAGS += -DPERF
FLAGS += -DLRU_CACHE
CXXFLAGS += -std=c++0x $(FLAGS)
CFLAGS += -std=c99 -msse4.2 $(FLAGS)

KAFKA_LIBS = -lrdkafka
CXXLIBS = -lboost_regex

OBJS = main.o kafkaConsumer.o userAgent.o \
            operatingSystem.o bot.o \
	        browser.o extractor.o enricher.o \
	        ipLocator.o wrapper.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: mafia json_test ua_test ip_test

mafia: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(CXXLIBS) $(KAFKA_LIBS)

# unit test

JSON_OBJS = test_json.o userAgent.o \
                operatingSystem.o bot.o \
                browser.o extractor.o enricher.o \
                ipLocator.o wrapper.o
json_test: $(JSON_OBJS)
	$(CXX) -o $@ $(JSON_OBJS) $(CXXLIBS)

UA_OBJS = test_ua.o userAgent.o \
                operatingSystem.o bot.o \
                browser.o extractor.o enricher.o \
                ipLocator.o wrapper.o
ua_test: $(UA_OBJS)
	$(CXX) -o $@ $(UA_OBJS) $(CXXLIBS)

IP_OBJS = test_ip.o userAgent.o \
                operatingSystem.o bot.o \
                browser.o extractor.o enricher.o \
                ipLocator.o wrapper.o
ip_test: $(IP_OBJS)
	$(CXX) -o $@ $(IP_OBJS) $(CXXLIBS)


.PHONY: clean
clean:
	rm -f *.o mafia json_test ua_test ip_test
