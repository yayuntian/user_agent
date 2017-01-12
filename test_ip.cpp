//
// Created by tyy on 2017/1/11.
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>

#include "ipLocator.h"
#include "wrapper.h"


void BenchmarkIP(const char *ip, int loop) {

    printf("loop cout: %u\n", loop);
    int i;
    struct timeval start, end;

    ipwrapper_init();

    gettimeofday(&start, NULL);
    for (i = 0; i < loop; i++) {
        char *js = ip2JsonStr(ip);
        if (i == 0) printf("%s\n", js);
    }
    gettimeofday(&end, NULL);

    long time_cost = ((end.tv_sec - start.tv_sec) * 1000000 + \
            end.tv_usec - start.tv_usec);
    printf("cost time: %ld us, %.2f pps\n", time_cost, loop / (time_cost * 1.0) * 1000000);
}


int main(int argc, char **argv) {
    int opt;
    int perf = 0;
    int loop = 100000;

    string ip = "211.147.1.25";

    while ((opt = getopt(argc, argv, "pc:h")) != -1) {
        switch (opt) {
            case 'p':
                perf = 1;
                break;
            case 'c':
                loop = atoi(optarg);
                break;
            case 'h':
            default:
                goto usage;
        }
    }

    if (perf == 1) {
        if (argc - optind > 0) {
            ip = argv[optind];
        }
        BenchmarkIP(ip.c_str(), loop);
        exit(0);
    }

    if (argc - optind > 0) {
        ipwrapper_init();
        for (int i = optind; i < argc; i++) {
            ip = argv[optind];
            char *js = ip2JsonStr(ip.c_str());
            printf("%s\n", js);
        }
        exit(0);
    }

usage:
    printf("Usage: %s <option> [ip1, ip2 ...]\n", argv[0]);
    printf("General options:\n"
           " -p              Perf test\n"
           " -c <count>      Perf test loop count\n"
           " -h              Show help\n"
    );
    return 0;
}

