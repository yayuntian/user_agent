#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

#include "IPWrapper.h"
#include "IPLocator.h"

using namespace boost::algorithm;


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


char jsonStr[1024];

char *toJsonStr(const string& str, const char *ipaddr, const char *raw) {

    string partten = "|";
    vector<string> v(10);
    split(v, str, is_any_of(partten));

    if (v.size() != 11) {
        fprintf(stderr, "ip split error\n");
        return NULL;
    }

    memset(jsonStr, 0, sizeof(jsonStr));

    char ipstr[32] = {0,};
    uint32_t i = strtoip(ipaddr);
    sprintf(ipstr, "%d", i);

    strcat(jsonStr, "{\"decimal\":");
    strcat(jsonStr, ipstr);

    strcat(jsonStr, ",\"raw\":");
    strcat(jsonStr, raw);

    strcat(jsonStr, ",\"dotted\":\"");
    strcat(jsonStr, ipaddr);

    strcat(jsonStr, ",\"region\":\"");
    strcat(jsonStr, v[3].c_str());

    strcat(jsonStr, ",\"isp\":\"");
    strcat(jsonStr, v[5].c_str());

    strcat(jsonStr, ",\"longtitude\":\"");
    strcat(jsonStr, v[9].c_str());

    strcat(jsonStr, ",\"latitude\":\"");
    strcat(jsonStr, v[10].c_str());
    strcat(jsonStr, "\"}\0");

    return jsonStr;
}


char *ipwrapper_query(const char *ip) {
    char ch[32] = {0,};
    const char *ipaddr = NULL;
    const char *raw = ip;

    const char *isdot = strstr(ip, ".");
    if (!isdot) {   // "1916214160" => 114.55.27.144
        iptostr(ch, 32, atoi(ip));
        ipaddr = ch;
    } else {    // "58.214.57.66" => 58.214.57.66
        int iplen = strlen(ip);
        strncpy(ch, ip + 1, iplen);
        ch[iplen - 2] = '\0';
        ipaddr = ch;
        raw = ch;
    }

    const char *result = finder->Query(ipaddr).c_str();
//    printf("ipaddr: %s => %s\n# result: %s\n",
//           ip, ipaddr, result);

    return toJsonStr(result, ipaddr, raw);
}


//int main()
//{
//    ipwrapper_init();
//
//    ipwrapper_query("12.25.26.35");
//
//    return 0;
//}