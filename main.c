#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#include "extractor.h"
#include "IPWrapper.h"

static char *buf_http = "{\n"
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
        "        \"src_ip\": \"58.214.57.66\", \n"
        "        \"xff\": \"\", \n"
        "        \"url\": \"/PHP/index.html\", \n"
        "        \"refer\": \"\", \n"
        "        \"l4_protocol\": \"tcp\", \n"
        "        \"in_pkts\": 1, \n"
        "        \"http_method\": 1, \n"
        "        \"out_pkts\": 6, \n"
        "        \"user_agent\": \"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) \\\"apple\\\":\\\"WebKit/537.36\\\" (KHTML, like Gecko) Chrome/43.0.2357.130 Safari/537.36 JianKongBao Monitor 1.1\", \n"
        "        \"dst_ip\": 1916214160, \n"
        "        \"https_flag\": 0, \n"
        "        \"src_port\": 43391, \n"
        "        \"latency_usec\": 498489, \n"
        "        \"host\": \"114.55.27.144\", \n"
        "        \"url_query\": \"\"\n"
        "    }, \n"
        "    \"probe_ts\": 1482978771, \n"
        "    \"dawn_ts1\": 1482978771547000, \n"
        "    \"topic\": \"cloudsensor\"\n"
        "}";


FILE *fp = NULL;

size_t write_data_log(const char *data, size_t length) {

    char *filename = "log.mafia";

    if (!fp) {
        if((fp = fopen (filename, "wb")) == NULL) {
            fprintf(stderr, "open file error: %s\n", filename);
            return 0;
        }
    }
    size_t ret = fwrite (data , sizeof(char), length, fp);

    return ret;
}


int ip_enricher(struct enrichee *enrichee__)
{
    char str[256] = {0,};

    strncpy(str, enrichee__->orig_value, enrichee__->orig_value_len);
    char *output = ip2JsonStr(str);
    printf("%s\n", output);
    return 0;
}

int ua_enricher(struct enrichee *enrichee__)
{
    char str[256] = {0,};

    strncpy(str, enrichee__->orig_value, enrichee__->orig_value_len);
    char *output = ua2JsonStr(str);
    printf("%s\n", output);

    return 0;
}


int main(int argc, char **argv)
{
    init();

    ipwrapper_init();

    register_enricher("src_ip", ip_enricher);
    register_enricher("dst_ip", ip_enricher);
    register_enricher("user_agent", ua_enricher);


    extract(buf_http, buf_http + strlen(buf_http));

    int i;
    for (i = 0; i < MAX_ENRICHEE; i++) {
        if (enrichees[i].orig_value_len == 0) {
            break;
        }

        char value[256] = {0,};
        strncpy(value, enrichees[i].orig_value, enrichees[i].orig_value_len);
        printf("## orig name: %s, len: %d\n", value, enrichees[i].orig_value_len);
    }
    return 0;
}
