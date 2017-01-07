#pragma once

#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "rdkafka.h"

/* POSIX */
#define RD_NORETURN __attribute__((noreturn))
#define RD_UNUSED __attribute__((unused))

#define CONF_F_OFFSET     0x4 /* Print offsets */
#define CONF_F_APIVERREQ  0x40 /* Enable api.version.request=true */
#define CONF_F_APIVERREQ_USER 0x80 /* User set api.version.request */

struct conf {
    int run;
    int verbosity;
    int exitcode;
    char mode;
    int flags;
    char *brokers;
    char *topic;
    int32_t partition;
    char *group;
    int64_t offset;
    int exit_eof;
    int64_t msg_cnt;

    rd_kafka_conf_t *rk_conf;
    rd_kafka_topic_conf_t *rkt_conf;

    rd_kafka_t *rk;
    rd_kafka_topic_t *rkt;

    char *debug;
    int conf_dump;
};

extern struct conf conf;


void RD_NORETURN fatal0(const char *func, int line,
                        const char *fmt, ...);

#define FATAL(.../*fmt*/)  fatal0(__FUNCTION__, __LINE__, __VA_ARGS__)

/* Info printout */
#define INFO(VERBLVL, ...)  \
        do {     \
            if (conf.verbosity >= (VERBLVL))     \
                fprintf(stderr, "%% " __VA_ARGS__); \
        } while (0)
