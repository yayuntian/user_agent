//
// Created by tyy on 2017/1/6.
//
#include <iostream>
#include <cstdlib>
#include <vector>
#include <unordered_map>

#include "lrucache.h"
#include "userAgent.h"


cache::lru_cache<string, UserAgent> cacheUA(1000);

void getSubStr(char *sub, const string ua, int start, int end) {
    if (end - start > 0) {
        const char *p = ua.c_str();
        strncpy(sub, p + start, end - start);
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

    if (cacheUA.exists(ua) == true) {
        p = cacheUA.get(ua);
        return;
    }

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

        cacheUA.put(ua, p);
    }
}



void echo_ua(UserAgent& p) {

    cout << p.ua << endl;

    cout << "browser Name: " << p.browser.Name << endl;
    cout << "browser Version: " << p.browser.Version << endl;
    cout << "browser Engine: " << p.browser.Engine << endl;
    cout << "browser EngineVersion: " << p.browser.EngineVersion << endl;

    cout << "os: " << p.os << endl;
    cout << "platfrom: " << p.platform << endl;
    cout << "local: " << p.localization << endl;
    cout << "mozilla: " << p.mozilla << endl;

    cout << "is bot: " << p.bot << endl;
    cout << "is mobile: " << p.mobile << endl;
    cout << "is undecided: " << p.undecided << endl;
}
