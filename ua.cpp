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

using namespace std;
using namespace boost::algorithm;

#include "ua.h"

using namespace std;


boost::regex ie11Regexp{"^rv:(.+)$"};


void detectBrowser(UserAgent& p, vector<Section>& sections) {

    int slen = sections.size();
    if (sections[0].name == "Opera") {
        p.browser.Name = "Opera";
        p.browser.Version = sections[0].version;
        p.browser.Engine = "Presto";
        if (slen > 1) {
            p.browser.EngineVersion = sections[1].version;
        }
    } else if (sections[0].name == "Dalvik") {
        p.mozilla = "5.0";
    } else if (slen > 1) {
        Section engine = sections[1];
        p.browser.Engine = engine.name;
        p.browser.EngineVersion = engine.version;
        if (slen > 2) {
            int sectionIndex = 2;
            if (sections[2].version == "" && slen > 3) {
                sectionIndex = 3;
            }
            p.browser.Version = sections[sectionIndex].version;
            if (engine.name == "AppleWebKit") {
                string name = sections[slen - 1].name;
                if (name == "Edge") {
                    p.browser.Name = "Edge";
                    p.browser.Version = sections[slen - 1].version;
                    p.browser.Engine = "EdgeHTML";
                    p.browser.EngineVersion = "";
                } else if (name == "OPR") {
                    p.browser.Name = "Opera";
                    p.browser.Version = sections[slen - 1].version;
                } else {
                    if (sections[sectionIndex].name == "Chrome") {
                        p.browser.Name = "Chrome";
                    } else if (sections[sectionIndex].name == "Chromium") {
                        p.browser.Name = "Chromium";
                    } else {
                        p.browser.Name = "Safari";
                    }
                }
            } else if (engine.name == "Gecko") {
                string name = sections[2].name;
                if (name == "MRA" && slen > 4) {
                    name = sections[4].name;
                    p.browser.Version = sections[4].version;
                }
                p.browser.Name = name;
            } else if (engine.name == "like" && sections[2].name == "Gecko") {
                p.browser.Engine = "Trident";
                p.browser.Name = "Internet Explorer";
                for (auto s : sections[0].comment) {
                    boost::smatch what;
                    if (boost::regex_search(s, what, ie11Regexp)) {
                        p.browser.Version = what[0];
                        return;
                    }
                }
                p.browser.Version = "";
            }
        }
    } else if (slen == 1 && sections[0].comment.size() > 1) {
        vector<string> comment = sections[0].comment;
        if (comment[0] == "compatible" && starts_with(comment[1], "MSIE")) {
            p.browser.Name = "Internet Explorer";
            p.browser.Engine = "Trident";

            size_t len = comment.size();
            for (size_t i = 0; i < len; i++) {
                if (starts_with(comment[i], "Trident/")) {
                    switch (atoi(comment[i].c_str())) {
                        case 4:
                            p.browser.Version = "8.0";
                            break;
                        case 5:
                            p.browser.Version = "9.0";
                            break;
                        case 6:
                            p.browser.Version = "10.0";
                            break;
                        default:
                            break;
                    }
                    break;
                }
            }
            if (p.browser.Version == "") {
                p.browser.Version = trim_copy(comment[1]);
            }
        }
    }
}


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

    }
}



void echo_ua(UserAgent& p) {

    cout << "raw: " << p.ua << endl;

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


int main(int argc, char **argv) {
    string ua = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.6; rv:2.0b8) Gecko/20100101 Firefox/4.0b8";
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