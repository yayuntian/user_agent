#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "extractor.h"

int ip_enricher(struct enrichee *enrichee__)
{
    return 0;
}

int ua_enricher(struct enrichee *enrichee__)
{
    return 0;
}

uint64_t rdtsc()
{
    unsigned long a, d;
    __asm__ __volatile__("cpuid; rdtsc" : "=a" (a), "=d" (d) : : "ebx", "ecx");
    return a | ((uint64_t)d << 32);
}

int main(int argc, char **argv)
{
    //char *buf = "{ \"foo\": \"bar\", \"src_ip\": \"127.0.0.1\", \"name\": 12345 }";
    char *buf =
"{"
"    \"dawn_ts0\": 1483522099153000, "
"    \"guid\": \"31\", "
"    \"device_id\": \"79bf7e53-d92f-5cdd-a7c3-3e9e97685c2c\", "
"    \"probe\": {"
"        \"name\": \"cloudsensor\""
"    }, "
"    \"appname\": \"cloudsensor\", "
"    \"type\": \"tcp\", "
"    \"kafka\": {"
"        \"topic\": \"cloudsensor\""
"    }, "
"    \"aggregate_count\": 1, "
"    \"tcp\": {"
"        \"src_isp\": 0, "
"        \"l4_proto\": 6, "
"        \"out_bytes\": 7364503, "
"        \"dst_port\": 3306, "
"        \"retransmitted_out_fin_pkts\": 0, "
"        \"client_latency_sec\": 0, "
"        \"window_zero_size\": 0, "
"        \"src_ipv4\": 178257969, "
"        \"topic\": \"tcp\", "
"        \"in_pkts\": 922, "
"        \"src_region\": 0, "
"        \"retransmitted_out_payload_pkts\": 5, "
"        \"dst_ipv4\": 178808905, "
"        \"ts\": 1483522099, "
"        \"final_status\": 3, "
"        \"retransmitted_in_syn_pkts\": 0, "
"        \"src_ip\": 178257969, "
"        \"server_latency_sec\": 0, "
"        \"l4_protocol\": 6, "
"        \"bytes_in\": 62654, "
"        \"src_port\": 58472, "
"        \"retransmitted_out_syn_pkts\": 0, "
"        \"retransmitted_in_ack_pkts\": 126, "
"        \"out_pkts\": 4871, "
"        \"device_id\": \"79bf7e53-d92f-5cdd-a7c3-3e9e97685c2c\", "
"        \"guid\": \"31\", "
"        \"bytes_out\": 7364503, "
"        \"retransmitted_in_payload_pkts\": 0, "
"        \"dst_ip\": 178808905, "
"        \"in_bytes\": 62654, "
"        \"retransmitted_out_ack_pkts\": 0, "
"        \"server_latency_usec\": 5055, "
"        \"client_latency_usec\": 6955, "
"        \"retransmitted_in_fin_pkts\": 0"
"    }, "
"    \"probe_ts\": 1483522099, "
"    \"dawn_ts1\": 1483522099153000, "
"    \"topic\": \"cloudsensor\""
"}";

    uint64_t begin, end;

    init();
    register_enricher("src_ip", ip_enricher);
    register_enricher("dst_ip", ip_enricher);
    register_enricher("user_agent", ua_enricher);
    begin = rdtsc();
    extract(buf, buf + strlen(buf));
    end = rdtsc();
    printf("Cost: %ld cycles.\n", end - begin);

    return 0;
}
