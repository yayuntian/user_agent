#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "IPWrapper.h"
#include "IPLocator.h"


IPSearch *finder;

void ipwrapper_init() {
    finder = IPSearch::instance();
    if (!finder) {
        printf("the IPSearch instance is null!");
        exit(1);
    }
}


static inline uint32_t strtoip(const char *ip)
{
    uint32_t ipaddr[4] = {0};
    sscanf(ip, "%u.%u.%u.%u", &ipaddr[0], &ipaddr[1], &ipaddr[2], &ipaddr[3]);
    return (ipaddr[0]<<24)|(ipaddr[1]<<16)|(ipaddr[2]<<8) | ipaddr[3];
}


static inline void iptostr(char ipstr[], uint16_t len, uint32_t ip)
{
    memset(ipstr, 0, len);
    sprintf(ipstr, "%u.%u.%u.%u",
            ip >> 24, (ip>>16&0xff), (ip>>8&0xff), (ip&0xff));
}



const char* ipwrapper_query(const char *ip) {
    char ch[32] = {0,};
    const char *ipaddr = NULL;

    const char *isdot = strstr(ip, ".");
    if (!isdot) {   // "1916214160" => 114.55.27.144
        iptostr(ch, 32, atoi(ip));
        ipaddr = ch;
    } else {    // "58.214.57.66" => 58.214.57.66
        int iplen = strlen(ip);
        strncpy(ch, ip + 1, iplen);
        ch[iplen - 2] = '\0';
        ipaddr = ch;
    }

    const char *result = finder->Query(ipaddr).c_str();
    printf("ipaddr: %s => %s\n# result: %s\n",
           ip, ipaddr, result);

    return result;
}


//int main()
//{
//    ipwrapper_init();
//
//    ipwrapper_query("12.25.26.35");
//
//    return 0;
//}