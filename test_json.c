//
// Created by tyy on 2017/1/11.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <sys/time.h>

#include "extractor.h"
#include "wrapper.h"


static char *jsonStr = "{\n"
        "    \"dawn_ts0\": 1483470142498000,\n"
        "    \"guid\": \"4a859fff6e5c4521aab187eee1cfceb8\",\n"
        "    \"device_id\": \"26aae27e-ffe5-5fc8-9281-f82cf4e288ee\",\n"
        "    \"probe\": {\n"
        "        \"name\": \"cloudsensor\",\n"
        "        \"hostname\": \"iZbp1gd3xwhcctm4ax2ruwZ\"\n"
        "    },\n"
        "    \"appname\": \"cloudsensor\",\n"
        "    \"type\": \"http\",\n"
        "    \"kafka\": {\n"
        "        \"topic\": \"cloudsensor\"\n"
        "    },\n"
        "    \"aggregate_count\": 1,\n"
        "    \"http\": {\n"
        "        \"latency_sec\": 0,\n"
        "        \"in_bytes\": 502,\n"
        "        \"status_code\": 200,\n"
        "        \"out_bytes\": 8625,\n"
        "        \"dst_port\": 80,\n"
        "        \"src_ip\": 2008838371,\n"
        "        \"xff\": \"\",\n"
        "        \"url\": \"/PHP/index.html\",\n"
        "        \"refer\": \"\",\n"
        "        \"l4_protocol\": \"tcp\",\n"
        "        \"in_pkts\": 1,\n"
        "        \"http_method\": 1,\n"
        "        \"out_pkts\": 6,\n"
        "        \"user_agent\": \"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.130 Safari/537.36 JianKongBao Monitor 1.1\",\n"
        "        \"dst_ip\": 1916214160,\n"
        "        \"https_flag\": 0,\n"
        "        \"src_port\": 43974,\n"
        "        \"latency_usec\": 527491,\n"
        "        \"host\": \"114.55.27.144\",\n"
        "        \"url_query\": \"\"\n"
        "    },\n"
        "    \"probe_ts\": 1483470142,\n"
        "    \"dawn_ts1\": 1483470142498000,\n"
        "    \"topic\": \"cloudsensor\"\n"
        "}";


void BenchmarkJson(const char *strJson, int loop, int func) {
    printf("Test json parser %s register func, loop cout: %u\n",
           func ? "contain" : "no", loop);

    int i;
    struct timeval start, end;
    char result[MAX_PAYLOAD_SIZE];

    init();
    ipwrapper_init();
    if (func == 1) {
        register_enricher("src_ip", ip_enricher);
        register_enricher("dst_ip", ip_enricher);
        register_enricher("user_agent", ua_enricher);
    }

    gettimeofday(&start, NULL);
    for (i = 0; i < loop; i++) {
        extract(strJson, strJson + strlen(strJson));
        memset(result, 0, MAX_PAYLOAD_SIZE);
        combine_enrichee(strJson, result);

        if (i == 0) printf("%s\n", result);
    }
    gettimeofday(&end, NULL);

    long time_cost = ((end.tv_sec - start.tv_sec) * 1000000 + \
            end.tv_usec - start.tv_usec);
    printf("cost time: %ld us, %.2f pps\n", time_cost, loop / (time_cost * 1.0) * 1000000);
}


int main(int argc, char **argv) {
    int opt;
    int perf = 0;
    int loop = 100000;
    int func = 0;

    while ((opt = getopt(argc, argv, "prc:h")) != -1) {
        switch (opt) {
            case 'p':
                perf = 1;
                break;
            case 'c':
                loop = atoi(optarg);
                break;
            case 'r':
                func = 1;
                break;
            case 'h':
            default:
                goto usage;
        }
    }

    if (perf == 1) {
        BenchmarkJson(jsonStr, loop, func);
        exit(0);
    }

usage:
    printf("Usage: %s <option> [user_agent]\n", argv[0]);
    printf("General options:\n"
           " -r              Call register func\n"
           " -p              Perf test\n"
           " -c <count>      Perf test loop count\n"
           " -h              Show help\n"
    );
    return 0;
}