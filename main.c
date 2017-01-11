#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>

#include "extractor.h"
#include "wrapper.h"
#include "kafkaConsumer.h"


#ifdef TEST_JSON
static char *buf_http = "{\n"
        "    \"src_ip\": 5689875,\n"
        "    \"dawn_ts0\": 1482978771547000, \n"
        "    \"guid\": \"4a859fff6e5c4521aab187eee1cfceb8\", \n"
        "    \"device_id\": \"26aae27e-ffe5-5fc8-9281-f82cf4e288ee\", \n"
        "    \"probe\": {\n"
        "        \"name\": \"cloudsensor\", \n"
        "        \"hostname\": \"iZbp1gd3xwhcctm4ax2ruwZ\"\n"
        "    }, \n"
        "    \"appname\": \"cloudsensor\", \n"
        "    \"type\": \"http\", \n"
        "    \"kafka\": {\n"
        "        \"topic\": \"cloudsensor\"\n"
        "    }, \n"
        "    \"aggregate_count\": 1, \n"
        "    \"http\": {\n"
        "        \"latency_sec\": 0, \n"
        "        \"in_bytes\": 502, \n"
        "        \"status_code\": 200, \n"
        "        \"out_bytes\": 8625, \n"
        "        \"dst_port\": 80, \n"
        "        \"src_ip\": 3661432842, \n"
        "        \"xff\": \"\", \n"
        "        \"url\": \"/PHP/index.html\", \n"
        "        \"refer\": \"\", \n"
        "        \"l4_protocol\": \"tcp\", \n"
        "        \"in_pkts\": 1, \n"
        "        \"http_method\": 1, \n"
        "        \"out_pkts\": 6, \n"
        "        \"user_agent\": \"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) appleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.130 Safari/537.36 JianKongBao Monitor 1.1\", \n"
        "        \"dst_ip\": 1916214160, \n"
        "        \"https_flag\": 0, \n"
        "        \"src_port\": 43391, \n"
        "        \"latency_usec\": 498489, \n"
        "        \"host\": \"114.55.27.144\", \n"
        "        \"url_query\": \"\"\n"
        "    }, \n"
        "    \"probe_ts\": 1482978771, \n"
        "    \"dawn_ts1\": 1482978771547000, \n"
        "    \"topic\": \"cloudsensor\",\n"
        "\t\"dst_ip\": \"192.168.10.12\",\n"
        "\t\"user_agent\": \"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2486.0 Safari/537.36 Edge/13.10586\"\n"
        "}";
#endif

FILE *fp = NULL;
static uint64_t rx_count = 0;

size_t write_data_log(const char *data, size_t length) {

    char *filename = (kconf.skip == 3) ? "/dev/null" : "log.mafia";

    if (!fp) {
        if ((fp = fopen(filename, "wb")) == NULL) {
            log_err("open file error: %s\n", filename);
            return 0;
        }
    }
    size_t ret = fwrite(data, sizeof(char), length, fp);
    fputs("\n", fp);

    return ret;
}


int ip_enricher(struct enrichee *enrichee__) {
    char value[MAX_ORIG_VAL_LEN] = {0,};

    strncpy(value, enrichee__->orig_value, enrichee__->orig_value_len);
    char *output = ip2JsonStr(value);

    if (!output) {
        enrichee__->orig_value_len = 0;
        enrichee__->enriched_value_len = 0;
        return 0;
    }

    int len = strlen(output);
    memset(enrichee__->enriched_value, 0, MAX_ENRICHED_VALUE_LEN);
    strncpy(enrichee__->enriched_value, output, len);
    enrichee__->enriched_value_len = len;

    return 0;
}

int ua_enricher(struct enrichee *enrichee__) {
    char value[MAX_ORIG_VAL_LEN] = {0,};

    strncpy(value, enrichee__->orig_value, enrichee__->orig_value_len);
    char *output = ua2JsonStr(value);

    if (!output) {
        enrichee__->orig_value_len = 0;
        enrichee__->enriched_value_len = 0;
        return 0;
    }

    int len = strlen(output);
    memset(enrichee__->enriched_value, 0, MAX_ENRICHED_VALUE_LEN);
    strncpy(enrichee__->enriched_value, output, len);
    enrichee__->enriched_value_len = len;
    return 0;
}



void combine_enrichee(const char *buf, char *result) {
    int i;
    int offset_buf = 0;
    int offset_result = 0;

    const char *next_clean_ptr;
    int next_clean_len;

    for (i = 0; i < MAX_ENRICHEE; i++) {
        if (enrichees[i].use == 0) {
            break;
        }
        enrichees[i].use = 0;

        // copy before i clean buf
        next_clean_ptr = buf + offset_buf;
        next_clean_len = enrichees[i].orig_value - (buf + offset_buf);

        strncpy(result + offset_result, next_clean_ptr, next_clean_len);

        offset_buf += next_clean_len + enrichees[i].orig_value_len;
        offset_result += next_clean_len;

        // copy i fix
        if (enrichees[i].enriched_value_len) {
            strncpy(result + offset_result, enrichees[i].enriched_value, enrichees[i].enriched_value_len);
            offset_result += enrichees[i].enriched_value_len;
        }
    }

    next_clean_ptr = buf + offset_buf;
    if (offset_buf < strlen(buf)) {
        next_clean_len = strlen(buf) - offset_buf;
        strncpy(result + offset_result, next_clean_ptr, next_clean_len);
    }
}


#ifdef TEST_JSON
int main(int argc, char **argv) {

    char result[MAX_PAYLOAD_SIZE];

    init();
    ipwrapper_init();

    register_enricher("src_ip", ip_enricher);
    register_enricher("dst_ip", ip_enricher);
    register_enricher("user_agent", ua_enricher);

    extract(buf_http, buf_http + strlen(buf_http));

    memset(result, 0, MAX_PAYLOAD_SIZE);
    combine_enrichee(buf_http, result);

    log(KLOG_DEBUG, "%s\n", result);

    return 0;
}
#endif

#ifdef PERF
void echo_perf() {
    long time_cost = ((kconf.end.tv_sec - kconf.start.tv_sec) * 1000000 + \
            kconf.end.tv_usec - kconf.start.tv_usec);

    fprintf(stderr, "# const time: %ld, rx cnt: %ld, byt: %ld\n",
            time_cost, kconf.msg_cnt, kconf.rx_byt);

    fprintf(stderr, "# %.2f pps, %.2f MBps\n",
            kconf.msg_cnt / (time_cost * 1.0) * 1000000,
            kconf.rx_byt / (time_cost * 1.0 * 1024 * 1024) * 1000000);
}
#endif


void payload_callback(rd_kafka_message_t *rkmessage) {

    char result[MAX_PAYLOAD_SIZE] = {0,};
    const char *buf = (char *)rkmessage->payload;
    const int buf_len = (int)rkmessage->len;

#ifdef PERF
    kconf.rx_byt += (int)rkmessage->len;
    if (rx_count == 0) {
        gettimeofday(&kconf.start, NULL);
    }
#endif
    if (++rx_count == kconf.msg_cnt) {
#ifdef PERF
        gettimeofday(&kconf.end, NULL);
        echo_perf();
#endif
        kconf.run = 0;
    }

    if (buf_len > MAX_PAYLOAD_SIZE) {
        log_err("payload size(%d) exceeds the threshold(%d)\n",
        buf_len, MAX_PAYLOAD_SIZE);
        write_data_log(buf, buf_len);
        return;
    }

    if (kconf.skip >= 2) {
        strncpy(result, buf, buf_len);
    } else {
        extract(buf, buf + buf_len);
        combine_enrichee(buf, result);
    }

    log(KLOG_DEBUG, "%s\n", result);
    write_data_log(result, strlen(result));
}


void read_file(void)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(kconf.filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Open file error\n");
        exit(1);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        line[read - 1] = '\0';
//        printf("[%zu]%s", read, line);

        rd_kafka_message_t msg_t;
        msg_t.payload = (void *) line;
        msg_t.len = read;
        payload_callback(&msg_t);

        if (kconf.run == 0) {
            break;
        }
    }

    fclose(fp);
    if (line) {
        free(line);
    }

    return;
}


struct kafkaConf kconf = {
    .run = 1,
    .msg_cnt = -1,
    .skip = 0,
    .filename = NULL,
    .verbosity = KLOG_INFO,
    .partition = RD_KAFKA_PARTITION_UA,
    .brokers = "localhost:9092",
    .group = "rdkafka_consumer_mafia",
    .topic_count = 0,
#ifdef PERF
    .rx_byt = 0,
#endif
    .payload_cb = payload_callback,
    .offset = RD_KAFKA_OFFSET_STORED
};


static void usage(const char *argv0) {

    printf("mafia - ClearClouds Message Tool\n"
    "Copyright (c) 2011-2017 WuXi Juyun System, Ltd.\n"
            "Version: 0.1beta\n"
            "\n");

    printf("Usage: %s <options> [topic1 topic2 ...]\n", argv0);

    printf("General options:\n"
            " -g <group>      Consumer group (%s)\n"
            " -b <brokes>     Broker address (%s)\n"
            " -f <file>       Consumer from json file\n"
            " -s <skip>       Skip process [test]\n"
            "                 1 - not regiest json cb, do parser, copy, write disk\n"
            "                 2 - not parser json, do copy, write disk\n"
            "                 3 - not parser json, do copy, write /dev/null\n"
            " -o <offset>     Offset to start consuming from:\n"
            "                 beginning[-2] | end[-1] | stored[-1000]\n"
            " -c <cnt>        Exit after consumering this number (-1)\n"
            " -q              Be quiet\n"
            " -e              Exit consumer when last message\n"
            " -d              Debug mode\n"
            " -h              Show help\n",
    kconf.group, kconf.brokers);

    exit(1);
}


/**
 * Parse command line arguments
 */
static void argparse (int argc, char **argv) {
    int i, opt;

    while ((opt = getopt(argc, argv, "g:b:s:o:c:f:qdh")) != -1) {
        switch (opt) {
            case 'b':
                kconf.brokers = optarg;
                break;
            case 'g':
                kconf.group = optarg;
                break;
            case 'f':
                kconf.filename = optarg;
                break;
            case 's':
                kconf.skip = atoi(optarg);
                break;
            case 'c':
                kconf.msg_cnt = strtoll(optarg, NULL, 10);
                break;
            case 'q':
                kconf.verbosity = KLOG_ERR;
                break;
            case 'd':
                kconf.verbosity = KLOG_DEBUG;
                break;
            case 'o':
                if (!strcmp(optarg, "end"))
                    kconf.offset = RD_KAFKA_OFFSET_END;
                else if (!strcmp(optarg, "beginning"))
                    kconf.offset = RD_KAFKA_OFFSET_BEGINNING;
                else if (!strcmp(optarg, "stored"))
                    kconf.offset = RD_KAFKA_OFFSET_STORED;
                else {
                    kconf.offset = strtoll(optarg, NULL, 10);
                    if (kconf.offset < 0)
                        kconf.offset = RD_KAFKA_OFFSET_TAIL(-kconf.offset);
                }
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    if (kconf.filename != NULL) {
        fprintf(stderr, "%% Read message from file: %s\n", kconf.filename);
        return;
    }

    kconf.topic_count = argc - optind;
    for (i = 0; i < kconf.topic_count; i++) {
        kconf.topic[i] = argv[optind + i];
    }

    if (!kconf.brokers || !kconf.group || !kconf.topic_count) {
        usage(argv[0]);
    }
}


int main(int argc, char **argv) {
    argparse(argc, argv);

    init();
    ipwrapper_init();

    if (kconf.skip < 1) {
        register_enricher("src_ip", ip_enricher);
        register_enricher("dst_ip", ip_enricher);
        register_enricher("user_agent", ua_enricher);
    }

    if (kconf.filename != NULL) {
        read_file();
        return 0;
    }

    init_kafka_consumer();

    return 0;
}
