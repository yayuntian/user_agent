//
// Created by tyy on 2017/1/9.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "ipLocator.h"
#include "userAgent.h"
#include "wrapper.h"


IPSearch *finder;

void ipwrapper_init() {
    finder = IPSearch::instance();
    if (!finder) {
        printf("the IPSearch instance is null!");
        exit(1);
    }
}


static inline uint32_t strtoip(const char *ip) {
    uint32_t ipaddr[4] = {0};
    sscanf(ip, "%u.%u.%u.%u", &ipaddr[0], &ipaddr[1], &ipaddr[2], &ipaddr[3]);
    return (ipaddr[0] << 24) | (ipaddr[1] << 16) | (ipaddr[2] << 8) | ipaddr[3];
}


static inline void iptostr(char ipstr[], uint16_t len, uint32_t ip) {
    memset(ipstr, 0, len);
    sprintf(ipstr, "%u.%u.%u.%u",
            ip >> 24, (ip >> 16 & 0xff), (ip >> 8 & 0xff), (ip & 0xff));
}

#define MAX_JSON_STR 2048
static char jsonStr[MAX_JSON_STR];

#define MAX_IP_STR 256
static char ipStr[MAX_IP_STR];


void ipStrSplit(const char *ipaddr, IPInfo &ipInfo) {
    char *p[16];
    int in = 0;
    char *buf = ipStr;
    memcpy(buf, ipaddr, strlen(ipaddr));

    char *out_ptr = NULL;
    while ((p[in] = strtok_r(buf, "|", &out_ptr)) != NULL) {
        in++;
        buf = NULL;
    }

    ipInfo.city = p[3];
    ipInfo.isp = p[5];
    ipInfo.longitude = p[9];
    ipInfo.latitude = p[10];
    return;
}


char *ip2JsonStr(const char *ip) {

    string partten = "|";
    char ch[32] = {0,};
    const char *ipaddr = NULL;
    const char *raw = ip;

    const char *isdot = strstr(ip, ".");
    if (!isdot) {   // "1916214160" => 114.55.27.144
        iptostr(ch, 32, atoi(ip));
        ipaddr = ch;
    } else {    // "58.214.57.66" => 58.214.57.66
        int iplen = strlen(ip);
        if (ip[0] == '\"') {
            strncpy(ch, ip + 1, iplen);
            ch[iplen - 2] = '\0';
            ipaddr = ch;
        } else {
            ipaddr = ip;
        }
    }

    IPInfo ipInfo;
    ipStrSplit(ipaddr, ipInfo);

    memset(jsonStr, 0, MAX_JSON_STR);

    char dec[32] = {0,};
    uint32_t i = strtoip(ipaddr);
    sprintf(dec, "%u", i);

    strcat(jsonStr, "{\"decimal\":");
    strcat(jsonStr, dec);

    strcat(jsonStr, ",\"raw\":");

    strcat(jsonStr, raw);

    strcat(jsonStr, ",\"dotted\":\"");
    strcat(jsonStr, ipaddr);

    strcat(jsonStr, "\",\"region\":\"");
    strcat(jsonStr, ipInfo.city.c_str());

    strcat(jsonStr, "\",\"isp\":\"");
    strcat(jsonStr, ipInfo.isp.c_str());

    strcat(jsonStr, "\",\"longtitude\":\"");
    strcat(jsonStr, ipInfo.longitude.c_str());

    strcat(jsonStr, "\",\"latitude\":\"");
    strcat(jsonStr, ipInfo.latitude.c_str());
    strcat(jsonStr, "\"}");

    return jsonStr;
}


char *ua2JsonStr(const char *ua, int ua_len) {

    UserAgent p;
    Parse(p, ua, ua_len);
//    echo_ua(p);

    memset(jsonStr, 0, MAX_JSON_STR);

    strcat(jsonStr, "{\"bot\":");
    if (p.bot) {
        strcat(jsonStr, "true");
    } else {
        strcat(jsonStr, "false");
    }

    strcat(jsonStr, ",\"browser\":");
    strcat(jsonStr, p.browser.Name.c_str());

    strcat(jsonStr, "\",\"browser_version\":\"");
    strcat(jsonStr, p.browser.Version.c_str());

    strcat(jsonStr, "\",\"engine\":\"");
    strcat(jsonStr, p.browser.Engine.c_str());

    strcat(jsonStr, "\",\"engine_version\":\"");
    strcat(jsonStr, p.browser.EngineVersion.c_str());

    strcat(jsonStr, "\",\"os\":\"");
    strcat(jsonStr, p.os.c_str());

    strcat(jsonStr, "\",\"platform\":\"");
    strcat(jsonStr, p.platform.c_str());

    strcat(jsonStr, "\",\"raw\":");
    strcat(jsonStr, ua);

    strcat(jsonStr, "}");

    return jsonStr;
}
