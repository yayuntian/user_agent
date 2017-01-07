//
// Created by tyy on 2017/1/6.
//
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

static int run = 1;
static rd_kafka_t *rk;
static int quiet = 0;

static void stop (int sig) {
	if (!run)
		exit(1);
	run = 0;
}


/**
 * Kafka logger callback (optional)
 */
static void logger (const rd_kafka_t *rk, int level,
		const char *fac, const char *buf) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	fprintf(stdout, "%u.%03u RDKAFKA-%i-%s: %s: %s\n",
			(int)tv.tv_sec, (int)(tv.tv_usec / 1000),
			level, fac, rd_kafka_name(rk), buf);
}


static void print_partition_list (
		const rd_kafka_topic_partition_list_t *partitions) {
	int i;
	for (i = 0 ; i < partitions->cnt ; i++) {
		fprintf(stderr, "%s [%d] offset %ld-",
				partitions->elems[i].topic,
				partitions->elems[i].partition,
				partitions->elems[i].offset);
	}
	fprintf(stderr, "\n");

}


static void rebalance_cb (rd_kafka_t *rk,
		rd_kafka_resp_err_t err,
		rd_kafka_topic_partition_list_t *partitions,
		void *opaque) {

	fprintf(stderr, "%% Consumer group rebalanced: ");

	switch (err)  {
		case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
			fprintf(stderr, "assigned:\n");
			print_partition_list(partitions);
			rd_kafka_assign(rk, partitions);

			break;

		case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
			fprintf(stderr, "revoked:\n");
			print_partition_list(partitions);
			rd_kafka_assign(rk, NULL);
			break;

		default:
			fprintf(stderr, "failed: %s\n",
					rd_kafka_err2str(err));
			rd_kafka_assign(rk, NULL);
			break;
	}
}


/**
 * Handle and print a consumed message.
 * Internally crafted messages are also used to propagate state from
 * librdkafka to the application. The application needs to check
 * the `rkmessage->err` field for this purpose.
 */
static void msg_consume (rd_kafka_message_t *rkmessage,
		void *opaque) {
	if (rkmessage->err) {
		if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
			fprintf(stderr,
					"%% Consumer reached end of %s %d "
					"message queue at offset %ld\n",
					rd_kafka_topic_name(rkmessage->rkt),
					rkmessage->partition, rkmessage->offset);

			fprintf(stderr, "%% Partition reached EOF: exiting\n");
			return;
		}

		if (rkmessage->rkt)
			fprintf(stderr, "%% Consume error for "
					"topic \"%s\" %d "
					"offset %ld: %s\n",
					rd_kafka_topic_name(rkmessage->rkt),
					rkmessage->partition,
					rkmessage->offset,
					rd_kafka_message_errstr(rkmessage));
		else
			fprintf(stderr, "%% Consumer error: %s: %s\n",
					rd_kafka_err2str(rkmessage->err),
					rd_kafka_message_errstr(rkmessage));

		if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
				rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC)
			run = 0;
		return;
	}

	if (!quiet) {
		fprintf(stdout, "%% Message (topic %s %d, "
				"offset %ld, %zd bytes):\n",
				rd_kafka_topic_name(rkmessage->rkt),
				rkmessage->partition,
				rkmessage->offset, rkmessage->len);
	}

	if (rkmessage->key_len) {
		printf("Key: %.*s\n",
				(int)rkmessage->key_len, (char *)rkmessage->key);
	}

	printf("%.*s\n", (int)rkmessage->len, (char *)rkmessage->payload);
}



int main(int argc, char **argv) {
	//char *brokers = "10.161.166.192:8301";
    char *brokers = "localhost:9200";
    char *group = NULL;

	int opt;
	rd_kafka_conf_t *conf;
	rd_kafka_topic_conf_t *topic_conf;
	char errstr[512];
	char tmp[16];
	rd_kafka_resp_err_t err;
	rd_kafka_topic_partition_list_t *topics;
	int is_subscription;
	int i;

	/* Kafka configuration */
	conf = rd_kafka_conf_new();

	/* Set logger */
	rd_kafka_conf_set_log_cb(conf, logger);

	/* Quick termination */
	snprintf(tmp, sizeof(tmp), "%i", SIGIO);
	rd_kafka_conf_set(conf, "internal.termination.signal", tmp, NULL, 0);

	/* Topic configuration */
	topic_conf = rd_kafka_topic_conf_new();

	while ((opt = getopt(argc, argv, "g:b:h")) != -1) {
		switch (opt) {
			case 'b':
				brokers = optarg;
				break;
			case 'g':
				group = optarg;
				break;
			case 'h':
			default:
                fprintf(stderr, "\"Usage: %s [options] topic\n"
                    "\n"
                    "  -g <group>      Consumer group (%s)\n"
                    "  -b <brokers>    Broker address (%s)\n",
                    argv[0], group, brokers);
                exit(1);
		}
	}

	signal(SIGINT, stop);

	if (!group) {
		group = "rdkafka_consumer_mafia";
	}
	if (rd_kafka_conf_set(conf, "group.id", group,
				errstr, sizeof(errstr)) !=
			RD_KAFKA_CONF_OK) {
		fprintf(stderr, "%% %s\n", errstr);
		exit(1);
	}

	/* Consumer groups always use broker based offset storage */
	if (rd_kafka_topic_conf_set(topic_conf, "offset.store.method",
				"broker",
				errstr, sizeof(errstr)) !=
			RD_KAFKA_CONF_OK) {
		fprintf(stderr, "%% %s\n", errstr);
		exit(1);
	}

	/* Set default topic config for pattern-matched topics. */
	rd_kafka_conf_set_default_topic_conf(conf, topic_conf);

	/* Callback called on partition assignment changes */
	rd_kafka_conf_set_rebalance_cb(conf, rebalance_cb);

	/* Create Kafka handle */
	if (!(rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf,
					errstr, sizeof(errstr)))) {
		fprintf(stderr,  "%% Failed to create new consumer: %s\n",
				errstr);
		exit(1);
	}

	rd_kafka_set_log_level(rk, 7);  //debug(7)

	/* Add brokers */
	if (rd_kafka_brokers_add(rk, brokers) == 0) {
		fprintf(stderr, "%% No valid brokers specified\n");
		exit(1);
	}

	/* Redirect rd_kafka_poll() to consumer_poll() */
	rd_kafka_poll_set_consumer(rk);

	topics = rd_kafka_topic_partition_list_new(argc - optind);
	is_subscription = 1;
	for (i = optind ; i < argc ; i++) {
		/* Parse "topic[:part] */
		char *topic = argv[i];
		char *t;
		int32_t partition = -1;

		if ((t = strstr(topic, ":"))) {
			*t = '\0';
			partition = atoi(t+1);
			is_subscription = 0; /* is assignment */
		}

		rd_kafka_topic_partition_list_add(topics, topic, partition);
	}

	if (is_subscription) {
		fprintf(stderr, "%% Subscribing to %d topics\n", topics->cnt);

		if ((err = rd_kafka_subscribe(rk, topics))) {
			fprintf(stderr,
					"%% Failed to start consuming topics: %s\n",
					rd_kafka_err2str(err));
			exit(1);
		}
	} else {
		fprintf(stderr, "%% Assigning %d partitions\n", topics->cnt);

		if ((err = rd_kafka_assign(rk, topics))) {
			fprintf(stderr,
					"%% Failed to assign partitions: %s\n",
					rd_kafka_err2str(err));
		}
	}

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
		fprintf(stderr, "%% Failed to close consumer: %s\n",
				rd_kafka_err2str(err));
	} else {
		fprintf(stderr, "%% Consumer closed\n");
	}


	rd_kafka_topic_partition_list_destroy(topics);

	/* Destroy handle */
	rd_kafka_destroy(rk);

	/* Let background threads clean up and terminate cleanly. */
	run = 5;
	while (run-- > 0 && rd_kafka_wait_destroyed(1000) == -1)
		printf("Waiting for librdkafka to decommission\n");
	if (run <= 0)
		rd_kafka_dump(stdout, rk);

	return 0;
}

