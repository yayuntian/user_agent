#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstdlib>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <cstring>

using namespace std;
using namespace boost::algorithm;

#include "ua.h"

using namespace std;


boost::regex ie11Regexp{"^rv:(.+)$"};


void detectBrowser(UserAgent& p, vector<Section> sections) {

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
        if (comment[0] == "compatible" &&
                strncmp(comment[1].c_str(), "MSIE", strlen("MSIE")) == 0) {
            p.browser.Name = "Internet Explorer";
            p.browser.Engine = "Trident";

            for (auto c : comment) {
                if (c == "4.0") {
                    p.browser.Version = "8.0";
                } else if (c == "5.0") {
                    p.browser.Version = "9.0";
                } else if (c == "6.0") {
                    p.browser.Version = "10.0";
                }
                break;
            }
            if (p.browser.Version == "") {
                p.browser.Version = trim_copy(comment[1]);
            }
        }
    }
}


string readUtil(string ua, int& index, char delimiter, bool cat) {
    string buffer;
    int catalan = 0;
    int len = ua.size();

    int i = index;

    for (; i < len; i++) {
        if (ua[i] == delimiter) {
            if (catalan == 0) {
                index = i + 1;
                return buffer;
            }
            catalan--;
        } else if (cat && ua[i] == '(') {
            catalan++;
        }
        buffer += ua[i];
    }
    index = i + 1;
    return buffer;
}


void parseProduct(Section& s, string product) {

    std::vector<std::string> v;
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


Section parseSection(string ua, int& index) {
    auto buffer = readUtil(ua, index, ' ', false);

    Section s;
    parseProduct(s, buffer);

    if (index < ua.size() && ua[index] == '(') {
        index++;
        buffer = readUtil(ua, index, ' ', true);
        split(s.comment, buffer, is_any_of("; "));
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
        //detectOS(p, sections[0]);

        if (p.undecided) {
            //checkBot(p, sections);
        }

    }
}

string Mozilla(UserAgent& p) {
    return p.mozilla;
}

string UA(UserAgent& p) {
    return p.ua;
}

bool Bot(UserAgent& p) {
    return p.bot;
}

bool Mobile(UserAgent& p) {
    return p.mobile;
}



int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}