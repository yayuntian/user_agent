//
// Created by tyy on 2017/1/9.
//

#ifndef MAFIA_IPWRAPPER_H
#define MAFIA_IPWRAPPER_H

#ifdef __cplusplus
extern "C"
{
#endif

    void ipwrapper_init();
    //char* ipwrapper_query(const char *ip);
    char *ip2JsonStr(const char *ip);
    char *ua2JsonStr(const char *ua, int len);


#ifdef __cplusplus
}
#endif

#endif //MAFIA_IPWRAPPER_H
