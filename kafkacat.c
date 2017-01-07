#include <unistd.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "kafkacat.h"


struct conf conf = {
        .run = 1,
        .verbosity = 1,
        .partition = RD_KAFKA_PARTITION_UA
};

static struct stats {
    uint64_t tx;
    uint64_t tx_err_q;
    uint64_t tx_err_dr;
    uint64_t tx_delivered;

    uint64_t rx;
} stats;


/* Partition's at EOF state array */
int *part_eof = NULL;
/* Number of partitions that has reached EOF */
int part_eof_cnt = 0;
/* Threshold level (partitions at EOF) before exiting */
int part_eof_thres = 0;



/**
 * Fatal error: print error and exit
 */
void RD_NORETURN fatal0 (const char *func, int line,
                         const char *fmt, ...) {
    va_list ap;
    char buf[1024];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    INFO(2, "Fatal error at %s:%i:\n", func, line);
    fprintf(stderr, "%% ERROR: %s\n", buf);
    exit(1);
}


void fmt_msg_output (FILE *fp, const rd_kafka_message_t *rkmessage) {

    fprintf(fp, "%% Message (topic %s %d, "
                    "offset %ld, %zd bytes):\n",
            rd_kafka_topic_name(rkmessage->rkt),
            rkmessage->partition,
            rkmessage->offset, rkmessage->len);

    if (rkmessage->key_len) {
        printf("Key: %.*s\n", (int)rkmessage->key_len, (char *)rkmessage->key);
    }

    printf("%.*s\n", (int)rkmessage->len, (char *)rkmessage->payload);
}


static void handle_partition_eof(rd_kafka_message_t *rkmessage) {

    if (conf.mode == 'C') {
        /* Store EOF offset.
         * If partition is empty and at offset 0,
         * store future first message (0). */
        rd_kafka_offset_store(rkmessage->rkt,
                              rkmessage->partition,
                              rkmessage->offset == 0 ?
                              0 : rkmessage->offset - 1);
        if (conf.exit_eof) {
            if (!part_eof[rkmessage->partition]) {
                /* Stop consuming this partition */
                rd_kafka_consume_stop(rkmessage->rkt,
                                      rkmessage->partition);
                part_eof[rkmessage->partition] = 1;
                part_eof_cnt++;
                if (part_eof_cnt >= part_eof_thres)
                    conf.run = 0;
            }
        }

    }

    INFO(1, "Reached end of topic %s [%d] at offset %ld%s\n",
         rd_kafka_topic_name(rkmessage->rkt),
         rkmessage->partition,
         rkmessage->offset,
         !conf.run ? ": exiting" : "");
}


/**
 * Consume callback, called for each message consumed.
 */
static void consume_cb(rd_kafka_message_t *rkmessage, void *opaque) {
    FILE *fp = opaque;

    if (!conf.run)
        return;

    if (rkmessage->err) {
        if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            handle_partition_eof(rkmessage);
            return;
        }

        FATAL("Topic %s [%d] error: %s",
              rd_kafka_topic_name(rkmessage->rkt),
              rkmessage->partition,
              rd_kafka_message_errstr(rkmessage));
    }

    /* Print message */
    fmt_msg_output(fp, rkmessage);

    if (conf.mode == 'C') {
        rd_kafka_offset_store(rkmessage->rkt,
                              rkmessage->partition,
                              rkmessage->offset);
    }

    if (++stats.rx == (uint64_t) conf.msg_cnt)
        conf.run = 0;
}


/**
 * Run consumer, consuming messages from Kafka and writing to 'fp'.
 */
static void consumer_run(FILE *fp) {
    char errstr[512];
    rd_kafka_resp_err_t err;
    const rd_kafka_metadata_t *metadata;
    int i;
    rd_kafka_queue_t *rkqu;

    /* Create consumer */
    if (!(conf.rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf.rk_conf,
                                 errstr, sizeof(errstr)))) {
        FATAL("Failed to create producer: %s", errstr);
    }


    if (!conf.debug && conf.verbosity == 0) {
        rd_kafka_set_log_level(conf.rk, 0);
    }


    /* The callback-based consumer API's offset store granularity is
     * not good enough for us, disable automatic offset store
     * and do it explicitly per-message in the consume callback instead. */
    if (rd_kafka_topic_conf_set(conf.rkt_conf,
                                "auto.commit.enable", "false",
                                errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        FATAL("%s", errstr);
    }


    /* Create topic */
    if (!(conf.rkt = rd_kafka_topic_new(conf.rk, conf.topic,
                                        conf.rkt_conf))) {
        FATAL("Failed to create topic %s: %s", conf.topic,
              rd_kafka_err2str(rd_kafka_errno2err(errno)));
    }


    conf.rk_conf = NULL;
    conf.rkt_conf = NULL;


    /* Query broker for topic + partition information. */
    if ((err = rd_kafka_metadata(conf.rk, 0, conf.rkt, &metadata, 5000))) {
        FATAL("Failed to query metadata for topic %s: %s",
              rd_kafka_topic_name(conf.rkt), rd_kafka_err2str(err));
    }


    /* Error handling */
    if (metadata->topic_cnt == 0) {
        FATAL("No such topic in cluster: %s",
              rd_kafka_topic_name(conf.rkt));
    }


    if ((err = metadata->topics[0].err)) {
        FATAL("Topic %s error: %s",
              rd_kafka_topic_name(conf.rkt), rd_kafka_err2str(err));
    }


    if (metadata->topics[0].partition_cnt == 0) {
        FATAL("Topic %s has no partitions",
              rd_kafka_topic_name(conf.rkt));
    }

    /* If Exit-at-EOF is enabled, set up array to track EOF
     * state for each partition. */
    if (conf.exit_eof) {
        part_eof = calloc(sizeof(*part_eof),
                          metadata->topics[0].partition_cnt);

        if (conf.partition != RD_KAFKA_PARTITION_UA) {
            part_eof_thres = 1;
        } else {
            part_eof_thres = metadata->topics[0].partition_cnt;
        }
    }

    /* Create a shared queue that combines messages from
     * all wanted partitions. */
    rkqu = rd_kafka_queue_new(conf.rk);

    /* Start consuming from all wanted partitions. */
    for (i = 0; i < metadata->topics[0].partition_cnt; i++) {
        int32_t partition = metadata->topics[0].partitions[i].id;

        /* If -p <part> was specified: skip unwanted partitions */
        if (conf.partition != RD_KAFKA_PARTITION_UA &&
            conf.partition != partition)
            continue;

        /* Start consumer for this partition */
        if (rd_kafka_consume_start_queue(conf.rkt, partition,
                                         conf.offset, rkqu) == -1) {
            FATAL("Failed to start consuming topic %s [%d]: %s",
                  conf.topic, partition,
                  rd_kafka_err2str(rd_kafka_errno2err(errno)));
        }


        if (conf.partition != RD_KAFKA_PARTITION_UA) {
            break;
        }
    }

    if (conf.partition != RD_KAFKA_PARTITION_UA &&
        i == metadata->topics[0].partition_cnt) {
        FATAL("Topic %s (with partitions 0..%i): "
                      "partition %i does not exist",
              rd_kafka_topic_name(conf.rkt),
              metadata->topics[0].partition_cnt - 1,
              conf.partition);
    }

    /* Read messages from Kafka, write to 'fp'. */
    while (conf.run) {
        rd_kafka_consume_callback_queue(rkqu, 100, consume_cb, fp);

        /* Poll for errors, etc */
        rd_kafka_poll(conf.rk, 0);
    }

    /* Stop consuming */
    for (i = 0; i < metadata->topics[0].partition_cnt; i++) {
        int32_t partition = metadata->topics[0].partitions[i].id;

        /* If -p <part> was specified: skip unwanted partitions */
        if (conf.partition != RD_KAFKA_PARTITION_UA &&
            conf.partition != partition) {
            continue;
        }

        /* Dont stop already stopped partitions */
        if (!part_eof || !part_eof[partition]) {
            rd_kafka_consume_stop(conf.rkt, partition);
        }

        rd_kafka_consume_stop(conf.rkt, partition);
    }

    /* Destroy shared queue */
    rd_kafka_queue_destroy(rkqu);

    /* Wait for outstanding requests to finish. */
    conf.run = 1;
    while (conf.run && rd_kafka_outq_len(conf.rk) > 0) {
        rd_kafka_poll(conf.rk, 50);
    }

    rd_kafka_metadata_destroy(metadata);
    rd_kafka_topic_destroy(conf.rkt);
    rd_kafka_destroy(conf.rk);
}


/**
 * Print metadata information
 */
static void metadata_print(const rd_kafka_metadata_t *metadata) {
    int i, j, k;

    printf("Metadata for %s (from broker %d: %s):\n",
           conf.topic ? conf.topic : "all topics",
           metadata->orig_broker_id, metadata->orig_broker_name);

    /* Iterate brokers */
    printf(" %i brokers:\n", metadata->broker_cnt);
    for (i = 0; i < metadata->broker_cnt; i++)
        printf("  broker %d at %s:%i\n",
               metadata->brokers[i].id,
               metadata->brokers[i].host,
               metadata->brokers[i].port);

    /* Iterate topics */
    printf(" %i topics:\n", metadata->topic_cnt);
    for (i = 0; i < metadata->topic_cnt; i++) {
        const rd_kafka_metadata_topic_t *t = &metadata->topics[i];
        printf("  topic \"%s\" with %i partitions:",
               t->topic,
               t->partition_cnt);
        if (t->err) {
            printf(" %s", rd_kafka_err2str(t->err));
            if (t->err == RD_KAFKA_RESP_ERR_LEADER_NOT_AVAILABLE)
                printf(" (try again)");
        }
        printf("\n");

        /* Iterate topic's partitions */
        for (j = 0; j < t->partition_cnt; j++) {
            const rd_kafka_metadata_partition_t *p;
            p = &t->partitions[j];
            printf("    partition %d, leader %d, replicas: ",
                   p->id, p->leader);

            /* Iterate partition's replicas */
            for (k = 0; k < p->replica_cnt; k++)
                printf("%s%d", k > 0 ? "," : "", p->replicas[k]);

            /* Iterate partition's ISRs */
            printf(", isrs: ");
            for (k = 0; k < p->isr_cnt; k++)
                printf("%s%d", k > 0 ? "," : "", p->isrs[k]);
            if (p->err)
                printf(", %s\n", rd_kafka_err2str(p->err));
            else
                printf("\n");
        }
    }
}


/**
 * Lists metadata
 */
static void metadata_list(void) {
    char errstr[512];
    rd_kafka_resp_err_t err;
    const rd_kafka_metadata_t *metadata;

    /* Create handle */
    if (!(conf.rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf.rk_conf,
                                 errstr, sizeof(errstr)))) {
        FATAL("Failed to create producer: %s", errstr);
    }

    if (!conf.debug && conf.verbosity == 0) {
        rd_kafka_set_log_level(conf.rk, 0);
    }


    /* Create topic, if specified */
    if (conf.topic &&
        !(conf.rkt = rd_kafka_topic_new(conf.rk, conf.topic,
                                        conf.rkt_conf))) {
        FATAL("Failed to create topic %s: %s", conf.topic,
              rd_kafka_err2str(rd_kafka_errno2err(errno)));
    }

    conf.rk_conf = NULL;
    conf.rkt_conf = NULL;

    /* Fetch metadata */
    err = rd_kafka_metadata(conf.rk, conf.rkt ? 0 : 1, conf.rkt,
                            &metadata, 5000);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        FATAL("Failed to acquire metadata: %s", rd_kafka_err2str(err));
    }

    /* Print metadata */
    metadata_print(metadata);

    rd_kafka_metadata_destroy(metadata);

    if (conf.rkt) {
        rd_kafka_topic_destroy(conf.rkt);
    }

    rd_kafka_destroy(conf.rk);
}


/**
 * Print usage and exit.
 */
static void RD_NORETURN usage(const char *argv0, int exitcode,
                              const char *reason,
                              int version_only) {
    FILE *out = stdout;
    if (reason) {
        out = stderr;
        fprintf(out, "Error: %s\n\n", reason);
    }

    if (!version_only) {
        fprintf(out, "Usage: %s <options> [file1 file2 .. | topic1 topic2 ..]]\n",
                argv0);
    }

    fprintf(out, "Copyright (c) 2014-2017, ClearClouds\n"
                    "Version %s (librdkafka %s)\n"
                    "\n", "0.1", rd_kafka_version_str());

    if (version_only) {
        exit(exitcode);
    }


    fprintf(out, "\n"
            "General options:\n"
            "  -C | -L            Mode: Consume or metadata List\n"
            "  -t <topic>         Topic to consume from, or list\n"
            "  -p <partition>     Partition\n"
            "  -b <brokers,..>    Bootstrap broker(s) (host[:port])\n"
            "  -c <cnt>           Limit message count\n"
            "  -X list            List available librdkafka configuration properties\n"
            "  -X prop=val        Set librdkafka configuration property.\n"
            "                     Properties prefixed with \"topic.\" are\n"
            "                     applied as topic properties.\n"
            "  -X dump            Dump configuration and exit.\n"
            "  -d <dbg1,...>      Enable librdkafka debugging:\n"
                                  RD_KAFKA_DEBUG_CONTEXTS
                                  "\n"
            "  -q                 Be quiet (verbosity set to 0)\n"
            "  -v                 Increase verbosity\n"
            "  -V                 Print version\n"
            "  -h                 Print usage help\n"
            "\n"
            "Consumer options:\n"
            "  -o <offset>        Offset to start consuming from:\n"
            "                     beginning | end | stored |\n"
            "                     <value>  (absolute offset) |\n"
            "                     -<value> (relative offset from end)\n"
            "  -e                 Exit successfully when last message received\n"
            "                     Takes precedence over -D and -K.\n"
            "  -O                 Print message offset using -K delimiter\n"
            "  -c <cnt>           Exit after consuming this number of messages\n"
            "  -Z                 Print NULL messages and keys as NULL"
            "(instead of empty)\n"
            "  -u                 Unbuffered output\n"
            "\n"
            " Example:\n"
            "  -f 'Topic %%t [%%p] at offset %%o: key %%k: %%s\\n'\n"
            "\n"
            "\n"
            "Consumer mode (writes messages to stdout):\n"
            "  mafia -b <broker> -t <topic> -p <partition>\n"
            "\n"
            "Metadata listing:\n"
            "  mafia -L -b <broker> [-t <topic>]\n"
            "\n"
    );
    exit(exitcode);
}


/**
 * Terminate by putting out the run flag.
 */
static void term(int sig) {
    conf.run = 0;
}


/**
 * librdkafka error callback
 */
static void error_cb(rd_kafka_t *rk, int err,
                     const char *reason, void *opaque) {

    if (err == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
        FATAL("%s: %s: terminating", rd_kafka_err2str(err),
              reason ? reason : "");
    }


    INFO(1, "ERROR: %s: %s\n", rd_kafka_err2str(err),
         reason ? reason : "");
}

/**
 * Parse command line arguments
 */
static void argparse(int argc, char **argv) {
    char errstr[512];
    int opt;

    while ((opt = getopt(argc, argv,
                         "CG:Lt:p:b:o:e:Od:qvX:c:Tu:Vh")) != -1) {
        switch (opt) {
            case 'C':
            case 'L':
                conf.mode = opt;
                break;
            case 't':
                conf.topic = optarg;
                break;
            case 'p':
                conf.partition = atoi(optarg);
                break;
            case 'b':
                conf.brokers = optarg;
                break;
            case 'o':
                if (!strcmp(optarg, "end"))
                    conf.offset = RD_KAFKA_OFFSET_END;
                else if (!strcmp(optarg, "beginning"))
                    conf.offset = RD_KAFKA_OFFSET_BEGINNING;
                else if (!strcmp(optarg, "stored"))
                    conf.offset = RD_KAFKA_OFFSET_STORED;
                else {
                    conf.offset = strtoll(optarg, NULL, 10);
                    if (conf.offset < 0)
                        conf.offset = RD_KAFKA_OFFSET_TAIL(-conf.offset);
                }
                break;
            case 'e':
                conf.exit_eof = 1;
                break;
            case 'O':
                conf.flags |= CONF_F_OFFSET;
                break;
            case 'c':
                conf.msg_cnt = strtoll(optarg, NULL, 10);
                break;
            case 'd':
                conf.debug = optarg;
                if (rd_kafka_conf_set(conf.rk_conf, "debug", conf.debug,
                                      errstr, sizeof(errstr)) !=
                    RD_KAFKA_CONF_OK)
                    FATAL("%s", errstr);
                break;
            case 'q':
                conf.verbosity = 0;
                break;
            case 'v':
                conf.verbosity++;
                break;
            case 'u':
                setbuf(stdout, NULL);
                break;
            case 'X': {
                char *name, *val;
                rd_kafka_conf_res_t res;

                if (!strcmp(optarg, "list") ||
                    !strcmp(optarg, "help")) {
                    rd_kafka_conf_properties_show(stdout);
                    exit(0);
                }

                if (!strcmp(optarg, "dump")) {
                    conf.conf_dump = 1;
                    continue;
                }

                name = optarg;
                if (!(val = strchr(name, '='))) {
                    fprintf(stderr, "%% Expected "
                            "-X property=value, not %s, "
                            "use -X list to display available "
                            "properties\n", name);
                    exit(1);
                }

                *val = '\0';
                val++;

                res = RD_KAFKA_CONF_UNKNOWN;
                /* Try "topic." prefixed properties on topic
                 * conf first, and then fall through to global if
                 * it didnt match a topic configuration property. */
                if (!strncmp(name, "topic.", strlen("topic.")))
                    res = rd_kafka_topic_conf_set(conf.rkt_conf,
                                                  name +
                                                  strlen("topic."),
                                                  val,
                                                  errstr,
                                                  sizeof(errstr));

                if (res == RD_KAFKA_CONF_UNKNOWN) {
                    res = rd_kafka_conf_set(conf.rk_conf, name, val,
                                            errstr, sizeof(errstr));
                }

                if (res != RD_KAFKA_CONF_OK)
                    FATAL("%s", errstr);
                if (!strcmp(name, "api.version.request"))
                    conf.flags |= CONF_F_APIVERREQ_USER;


            }
                break;

            case 'V':
                usage(argv[0], 0, NULL, 1);
                break;

            case 'h':
                usage(argv[0], 0, NULL, 0);
                break;

            default:
                usage(argv[0], 1, "unknown argument", 0);
                break;
        }
    }


    if (!conf.brokers) {
        usage(argv[0], 1, "-b <broker,..> missing", 0);
    }


    /* Decide mode if not specified */
    if (!conf.mode) {
        conf.mode = 'L';
        INFO(1, "Auto-selecting %s mode (use -L or -C to override)\n",
             conf.mode == 'C' ? "Consumer" : "List");
    }

    if (!strchr("L", conf.mode) && !conf.topic) {
        usage(argv[0], 1, "-t <topic> missing", 0);
    }


    if (rd_kafka_conf_set(conf.rk_conf, "metadata.broker.list",
        conf.brokers, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        usage(argv[0], 1, errstr, 0);
    }

    rd_kafka_conf_set_error_cb(conf.rk_conf, error_cb);

    /* Automatically enable API version requests if needed and
     * user hasn't explicitly configured it (in any way). */
    if ((conf.flags & (CONF_F_APIVERREQ | CONF_F_APIVERREQ_USER)) ==
        CONF_F_APIVERREQ) {
        INFO(2, "Automatically enabling api.version.request=true\n");
        rd_kafka_conf_set(conf.rk_conf, "api.version.request", "true", NULL, 0);
    }
}


/**
 * Dump current rdkafka configuration to stdout.
 */
static void conf_dump(void) {
    const char **arr;
    size_t cnt;
    int pass;

    for (pass = 0; pass < 2; pass++) {
        int i;

        if (pass == 0) {
            arr = rd_kafka_conf_dump(conf.rk_conf, &cnt);
            printf("# Global config\n");
        } else {
            printf("# Topic config\n");
            arr = rd_kafka_topic_conf_dump(conf.rkt_conf, &cnt);
        }

        for (i = 0; i < (int) cnt; i += 2) {
            printf("%s = %s\n", arr[i], arr[i + 1]);
        }
        printf("\n");

        rd_kafka_conf_dump_free(arr, cnt);
    }
}


int main(int argc, char **argv) {
#ifdef SIGIO
    char tmp[16];
#endif
    signal(SIGINT, term);
    signal(SIGTERM, term);
#ifdef SIGPIPE
    signal(SIGPIPE, term);
#endif
    /* Create config containers */
    conf.rk_conf = rd_kafka_conf_new();
    conf.rkt_conf = rd_kafka_topic_conf_new();

    /*
     * Default config
     */
#ifdef SIGIO
    /* Enable quick termination of librdkafka */
        snprintf(tmp, sizeof(tmp), "%i", SIGIO);
        rd_kafka_conf_set(conf.rk_conf, "internal.termination.signal",
                          tmp, NULL, 0);
#endif

    /* Log callback */
    rd_kafka_conf_set_log_cb(conf.rk_conf, rd_kafka_log_print);

    /* Parse command line arguments */
    argparse(argc, argv);

    /* Dump configuration and exit, if so desired. */
    if (conf.conf_dump) {
        conf_dump();
        exit(0);
    }

    /* Run according to mode */
    switch (conf.mode) {
        case 'C':
            consumer_run(stdout);
            break;
        case 'L':
            metadata_list();
            break;
        default:
            usage(argv[0], 0, NULL, 0);
            break;
    }

    rd_kafka_wait_destroyed(5000);

    exit(conf.exitcode);
}