#ifndef MAFIA_EXTRACTOR_H
#define MAFIA_EXTRACTOR_H

#define MAX_INTERESTED_PAIRS 8
#define MAX_ENRICHEE 32
#define MAX_ORIG_NAME_LEN 128
#define MAX_ENRICHED_VALUE_LEN 4096
#define MAX_PAYLOAD_SIZE    8192

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

struct enrichee {
    int orig_name_len;
    const char *orig_name;

    int orig_value_len;
    const char *orig_value; // 如果是string的话包括双引号

    int enriched_value_len;
    char *enriched_value;

    int use;
};

typedef int (*enricher)(struct enrichee *enrichee__);

struct interested_pair {
    int name_len;
    char *name;
    enricher enricher__;
};


extern struct enrichee enrichees[MAX_ENRICHEE];


int init();
int register_enricher(const char *interested_name, enricher enricher__);
int extract(const char *buf, const char *buf_end);

#endif // MAFIA_EXTRACTOR_H
