//
// Created by tyy on 2017/1/9.
//

#ifndef MAFIA_KAFKACONSUMER_H
#define MAFIA_KAFKACONSUMER_H

#include "rdkafka.h"


#define KLOG_ERR 1
#define KLOG_WAR 2
#define KLOG_INFO 3
#define KLOG_DEBUG 4

typedef void (*kafka_payload_cb)(rd_kafka_message_t *rkmessage);

struct kafkaConf {
    int run;
    int verbosity;

    char *brokers;
    char *group;
    char *topic;
    int32_t topic_count;
    int32_t partition;
    int64_t offset;

    rd_kafka_conf_t *rk_conf;
    rd_kafka_topic_conf_t *rkt_conf;

    rd_kafka_t *rk;
    rd_kafka_topic_partition_list_t *rktp;

    kafka_payload_cb payload_cb;
};

extern struct kafkaConf kconf;

int init_kafka_consumer(void);


#define log_err(fmt, args...)   fprintf(stderr, "%s: " fmt, __func__, ##args)
#define log(VER, fmt, args...)  \
    do {    \
        if (kconf.verbosity >= (VER)) {  \
            printf(fmt, ##args);    \
        }   \
    }while (0);

#endif //MAFIA_KAFKACONSUMER_H
