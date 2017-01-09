#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>
#include <string>
#include <cstring>

#include "userAgent.h"
#include "lrucache.h"

using namespace std;
using namespace boost::algorithm;

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

    std::vector<std::string> v(2);
    split(v, product, is_any_of("/"));

    if (v.size() == 2) {
        s.name = v[0];
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

    vector<Section> sections(10);
    initialize(p);
    p.ua = ua;

    int index = 0;
    int limit = ua.size();
    int count = 0;

    while (index < limit) {
        Section s = parseSection(ua, index);
        if (!p.mobile && s.name == "Mobile") {
            p.mobile = true;
        }
//        sections.push_back(s);
        sections[count++] = s;
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

#if 0
int test_ua_performance(int argc, char **argv) {
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
#endif