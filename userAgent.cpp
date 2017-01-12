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


string readUtil(string& ua, int& index, char delimiter, bool cat) {
    int catalan = 0;
    int len = ua.size();
    int i = index;
    char buf[256] = {0,};

    for (; i < len; i++) {
        if (ua[i] == delimiter) {
            if (catalan == 0) {
                getSubStr(buf, ua, index, i);
                index = i + 1;
                return buf;
            }
            catalan--;
        } else if (cat && ua[i] == '(') {
            catalan++;
        }
    }
    getSubStr(buf, ua, index, i);
    index = i + 1;
    return buf;
}


void parseProduct(Section& s, string product) {

    std::vector<std::string> v;
    split(v, product, is_any_of("/"));

    if (v.size() == 2) {
        s.name = replace_all_copy(v[0], "\\", "");
        s.version = v[1];
        return;
    } else {
        s.name = product;
        s.version = "";
    }
}


Section parseSection(string& ua, int& index) {
    string buffer = readUtil(ua, index, ' ', false);

    Section s;
    parseProduct(s, buffer);

    if (index < (int)ua.size() && ua[index] == '(') {
        index++;
        buffer = readUtil(ua, index, ')', true);
        //split(s.comment, buffer, is_any_of("; "));
        split_regex(s.comment, buffer, boost::regex("; ")) ;
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


void Parse(UserAgent& p, string ua) {

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
    int limit = ua.size();

    while (index < limit) {
        Section s = parseSection(ua, index);
        if (!p.mobile && s.name == "Mobile") {
            p.mobile = true;
        }
        sections.push_back(s);
    }

    if (sections.size() > 0) {
        if (sections[0].name == "Mozilla") {
            p.mozilla = sections[0].version;
        }

        detectBrowser(p, sections);
        detectOS(p, sections[0]);

        if (p.undecided) {
            checkBot(p, sections);
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