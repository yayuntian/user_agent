//
// Created by tyy on 2017/1/9.
//

#ifndef MAFIA_KAFKACONSUMER_H
#define MAFIA_KAFKACONSUMER_H

#include "rdkafka.h"

#define log_err(fmt, args...)   fprintf(stderr, fmt, ##args)
#define log_info(fmt, args...)  printf(fmt, ##args)

typedef void (*kafka_payload_cb)(rd_kafka_message_t *rkmessage);

struct kafkaConf {
    int run;

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

#endif //MAFIA_KAFKACONSUMER_H
