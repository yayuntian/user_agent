//
// Created by tyy on 2017/1/11.
//
#include "userAgent.h"


int main(int argc, char **argv) {
    string ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
        " (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.10240";
    if (argc < 2) {
        printf("Usage: %s  <loop-count>\n", argv[0]);
        exit(0);
    } else if (argc == 3) {
        ua = argv[2];
    }

    int i;
    int loop = atoi(argv[1]);

    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (i = 0; i < loop; i++) {
        UserAgent p;
        Parse(p, ua);
        if (i == 0) {
            echo_ua(p);
        }

    }
    gettimeofday(&end, NULL);

    long time_cost = ((end.tv_sec - start.tv_sec) * 1000000 + \
            end.tv_usec - start.tv_usec);

    printf("cost time: %ld us, %.2f pps\n", time_cost, loop / (time_cost * 1.0) * 1000000);

    return 0;
}

