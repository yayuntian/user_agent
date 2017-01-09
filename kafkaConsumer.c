/**
 * Apache Kafka high level consumer example program
 * using the Kafka driver from librdkafka
 * (https://github.com/edenhill/librdkafka)
 */

#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/time.h>
#include <errno.h>
#include <getopt.h>

#include "rdkafka.h"

#define log_err(fmt, args...)   fprintf(stderr, fmt, ##args)
#define log_info(fmt, args...)  printf(fmt, ##args)


static int run = 1;
const static uint64_t msg_count = 10;
static uint64_t rx_count = 0;

static void stop (int sig) {
    if (!run) {
        exit(1);
    }
    run = 0;
}


/**
 * Kafka logger callback (optional)
 */
static void logger (const rd_kafka_t *rk, int level,
                    const char *fac, const char *buf) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    log_info("%u.%03u RDKAFKA-%i-%s: %s: %s\n",
            (int)tv.tv_sec, (int)(tv.tv_usec / 1000),
            level, fac, rd_kafka_name(rk), buf);
}



/**
 * Handle and print a consumed message.
 * Internally crafted messages are also used to propagate state from
 * librdkafka to the application. The application needs to check
 * the `rkmessage->err` field for this purpose.
 */
static void msg_consume (rd_kafka_message_t *rkmessage,
                         void *opaque) {
    if (run == 0) {
        return;
    }

    if (rkmessage->err) {
        if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            log_err("%% Consumer reached end of %s [%d] "
                            "message queue at offset %ld\n",
                    rd_kafka_topic_name(rkmessage->rkt),
                    rkmessage->partition, rkmessage->offset);
            return;
        }

        if (rkmessage->rkt) {
            log_err("%% Consume error for "
                            "topic \"%s\" [%d] "
                            "offset %ld: %s\n",
                    rd_kafka_topic_name(rkmessage->rkt),
                    rkmessage->partition,
                    rkmessage->offset,
                    rd_kafka_message_errstr(rkmessage));
        } else {
            log_err("%% Consumer error: %s: %s\n",
                rd_kafka_err2str(rkmessage->err),
                rd_kafka_message_errstr(rkmessage));
        }

        if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
            rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC) {
            run = 0;
        }

        return;
    }

    log_err("%% Message (topic %s [%d], "
                    "offset %ld, %zd bytes):\n",
            rd_kafka_topic_name(rkmessage->rkt),
            rkmessage->partition,
            rkmessage->offset, rkmessage->len);

    if (rkmessage->key_len) {
        log_info("Key: %.*s\n", (int)rkmessage->key_len, (char *)rkmessage->key);
    }

    //log_info("%.*s\n", (int)rkmessage->len, (char *)rkmessage->payload);

    if (++rx_count == msg_count) {
        run = 0;
    }

}


static void print_partition_list (FILE *fp,
                                  const rd_kafka_topic_partition_list_t
                                  *partitions) {
    int i;
    for (i = 0 ; i < partitions->cnt ; i++) {
        log_err("%s %s [%d] offset %ld",
                i > 0 ? ",":"",
                partitions->elems[i].topic,
                partitions->elems[i].partition,
                partitions->elems[i].offset);
    }
    log_err("\n");

}


static RD_UNUSED void set_partition_offset (rd_kafka_topic_partition_list_t
                                  *partitions, const int64_t offset) {
    int i;
    for (i = 0 ; i < partitions->cnt ; i++) {
        rd_kafka_topic_partition_t *part;
        char *topic = partitions->elems[i].topic;
        int32_t partition = partitions->elems[i].partition;
        if ((part = rd_kafka_topic_partition_list_find(partitions, topic, partition))) {
            part->offset = offset;
        } else {
            log_err("set partition offset error: %s[%d] offset:%ld\n",
            topic, partition, offset);
        }
    }
}


static void rebalance_cb (rd_kafka_t *rk,
                          rd_kafka_resp_err_t err,
                          rd_kafka_topic_partition_list_t *partitions,
                          void *opaque) {

    log_err("%% Consumer group rebalanced: ");

    switch (err)  {
        case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
            log_err("assigned:\n");

            set_partition_offset(partitions, RD_KAFKA_OFFSET_STORED);

            print_partition_list(stderr, partitions);
            rd_kafka_assign(rk, partitions);
            break;

        case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
            log_err("revoked:\n");
            print_partition_list(stderr, partitions);
            rd_kafka_assign(rk, NULL);
            break;

        default:
            log_err("failed: %s\n", rd_kafka_err2str(err));
            rd_kafka_assign(rk, NULL);
            break;
    }
}


int main (int argc, char **argv) {
    static rd_kafka_t *rk;
    char *brokers = "10.161.166.192:8301";
    char *group = "testkafka-offset-begin";

    // topic == topic_count
    char *topic = "cloudsensor";
    int32_t topic_count = 1;

    int32_t partition = RD_KAFKA_PARTITION_UA;
    rd_kafka_conf_t *conf;
    rd_kafka_topic_conf_t *topic_conf;
    char errstr[512];
    char tmp[16];
    rd_kafka_resp_err_t err;

    rd_kafka_topic_partition_list_t *topics;

    /* Kafka configuration */
    conf = rd_kafka_conf_new();

    /* Set logger */
    rd_kafka_conf_set_log_cb(conf, logger);

    /* Quick termination */
    snprintf(tmp, sizeof(tmp), "%i", SIGIO);
    rd_kafka_conf_set(conf, "internal.termination.signal", tmp, NULL, 0);

    /* Topic configuration */
    topic_conf = rd_kafka_topic_conf_new();

    signal(SIGINT, stop);

    /* Consumer groups require a group id */
    if (!group) {
        group = "rdkafka_consumer_example";
    }

    if (rd_kafka_conf_set(conf, "group.id", group,
                          errstr, sizeof(errstr)) !=
        RD_KAFKA_CONF_OK) {
        log_err("%% %s\n", errstr);
        exit(1);
    }

    /* Consumer groups always use broker based offset storage */
    if (rd_kafka_topic_conf_set(topic_conf, "offset.store.method",
                                "broker",
                                errstr, sizeof(errstr)) !=
        RD_KAFKA_CONF_OK) {
        log_err("%% %s\n", errstr);
        exit(1);
    }

    /* Set default topic config for pattern-matched topics. */
    rd_kafka_conf_set_default_topic_conf(conf, topic_conf);

    /* Callback called on partition assignment changes */
    rd_kafka_conf_set_rebalance_cb(conf, rebalance_cb);

    /* Create Kafka handle */
    if (!(rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf,
                            errstr, sizeof(errstr)))) {
        log_err("%% Failed to create new consumer: %s\n",
                errstr);
        exit(1);
    }

    rd_kafka_set_log_level(rk, LOG_DEBUG);

    /* Add brokers */
    if (rd_kafka_brokers_add(rk, brokers) == 0) {
        log_err("%% No valid brokers specified\n");
        exit(1);
    }

    /* Redirect rd_kafka_poll() to consumer_poll() */
    rd_kafka_poll_set_consumer(rk);


    // fix topic count
    topics = rd_kafka_topic_partition_list_new(topic_count);
    rd_kafka_topic_partition_list_add(topics, topic, partition);

    log_err("%% Subscribing to %d topics\n", topics->cnt);

    if ((err = rd_kafka_subscribe(rk, topics))) {
        log_err("%% Failed to start consuming topics: %s\n",
                rd_kafka_err2str(err));
        exit(1);
    }
    rd_kafka_topic_partition_list_destroy(topics);

    while (run) {
        rd_kafka_message_t *rkmessage;

        rkmessage = rd_kafka_consumer_poll(rk, 1000);
        if (rkmessage) {
            msg_consume(rkmessage, NULL);
            rd_kafka_message_destroy(rkmessage);
        }
    }

    err = rd_kafka_consumer_close(rk);
    if (err) {
        log_err("%% Failed to close consumer: %s\n",
                rd_kafka_err2str(err));
    } else {
        log_err("%% Consumer closed\n");
    }

    /* Destroy handle */
    rd_kafka_destroy(rk);

    /* Let background threads clean up and terminate cleanly. */
    run = 5;
    while (run-- > 0 && rd_kafka_wait_destroyed(1000) == -1)
        log_info("Waiting for librdkafka to decommission\n");
    if (run <= 0)
        rd_kafka_dump(stdout, rk);

    return 0;
}