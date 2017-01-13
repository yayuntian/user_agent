//
// Created by tyy on 2017/1/6.
//
#include <iostream>
#include <cstdlib>
#include <vector>
#include <unordered_map>

#include "lrucache.h"
#include "userAgent.h"

#ifdef LRU_CACHE
cache::lru_cache<string, UserAgent> cacheUA(1000);
#endif

void getSubStr(char *sub, const string ua, int start, int end) {
    size_t len = end - start;
    if (len > 0) {
        const char *p = ua.c_str();
        strncpy(sub, p + start, len);
    }
}


void readUtil(string& ua, int ua_len, char *buf, int& index, char delimiter, bool cat) {
    int catalan = 0;
    int i = index;

    for (; i < ua_len; i++) {
        if (ua[i] == delimiter) {
            if (catalan == 0) {
                getSubStr(buf, ua, index, i);
                index = i + 1;
                return;
            }
            catalan--;
        } else if (cat && ua[i] == '(') {
            catalan++;
        }
    }
    getSubStr(buf, ua, index, i);
    index = i + 1;
}


void parseProduct(Section& s, char *product) {

    char *ver = NULL;
    char *ptr = strtok_r(product, "/", &ver);

    // remove json '\' character, ex: Mozilla\/5.0
    remove_rchar(product);

    s.name = product;
    if (ptr != NULL) {
        s.version = ver;
    } else {
        s.version = "";
    }
}


Section parseSection(string& ua, int ua_len, int& index) {
    char buf[512] = {0,};

    readUtil(ua, ua_len, buf, index, ' ', false);

    Section s;
    parseProduct(s, buf);

    if (index < ua_len && ua[index] == '(') {
        index++;
        memset(buf, 0, 512);
        readUtil(ua, ua_len, buf, index, ')', true);
        split_regex(s.comment, buf, boost::regex("; ")) ;
        index++;
    }

    return s;
}


void initialize(UserAgent& p) {
    p.ua = "";
    p.mozilla = "";
    p.platform = "";
    p.os = "";
    p.localization = "";
    p.browser.Engine = "";
    p.browser.EngineVersion = "";
    p.browser.Name = "";
    p.browser.Version = "";
    p.bot = false;
    p.mobile = false;
    p.undecided = false;
}


void Parse(UserAgent& p, string ua, const int ua_len) {

#ifdef LRU_CACHE
    if (cacheUA.exists(ua) == true) {
        p = cacheUA.get(ua);
        return;
    }
#endif
    vector<Section> sections;
    initialize(p);
    p.ua = ua;

    int index = 0;

    while (index < ua_len) {
        Section s = parseSection(ua, ua_len, index);
        if (!p.mobile && s.name == "Mobile") {
            p.mobile = true;
        }
        sections.push_back(s);
    }

    int slen = sections.size();
    if (slen > 0) {
        if (sections[0].name == "Mozilla") {
            p.mozilla = sections[0].version;
        }

        detectBrowser(p, sections, slen);
        detectOS(p, sections[0], sections[0].comment.size());

        if (p.undecided) {
            checkBot(p, sections, slen);
        }
#ifdef LRU_CACHE
        cacheUA.put(ua, p);
#endif
    }
}


void print_val(string key, string val) {
    if (!val.empty()) {
        printf("%s: %s\n", key.c_str(), val.c_str());
    }
}

void print_val(string key, bool val) {
    if (val) {
        printf("%s: %s\n", key.c_str(), "true");
    } else {
        printf("%s: %s\n", key.c_str(), "false");
    }
}


void echo_ua(UserAgent& p) {

    cout << p.ua << endl;
    if (!p.browser.Name.empty()) {
        printf("browser Name: %s, Version: %s\n",
               p.browser.Name.c_str(),
               p.browser.Version.c_str());
    }
    if (!p.browser.Engine.empty()) {
        printf("browser Engine: %s, Version: %s\n",
               p.browser.Engine.c_str(),
               p.browser.EngineVersion.c_str());
    }

    print_val("os", p.os);
    print_val("platform", p.platform);
    print_val("localization", p.localization);
    print_val("mozilla", p.mozilla);

    print_val("bot", p.bot);
    print_val("mobile", p.mobile);
}
