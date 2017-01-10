//
// Created by tyy on 2017/1/5.
//
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

using namespace std;
using namespace boost::algorithm;


boost::regex botFromSiteRegexp{"http://.+\\.\\w+"};
boost::regex botRegexp{"(?i)(bot|crawler|sp(i|y)der|search|worm|fetch|nutch)"};


string getFromSite(vector<string>& comment) {
    size_t slen = comment.size();

    if (slen == 0) {
        return "";
    }

    int idx = (slen < 3) ? 0 : 2;
    boost::smatch result;

    string ret = "";
    int count = 0;

    std::string::const_iterator start = comment[idx].begin();
    std::string::const_iterator end = comment[idx].end();

    while ( boost::regex_search(start, end, result, botFromSiteRegexp) ) {
        if (count++ == 0) {
            ret = result[0];
        }
        start = result[0].second;
    }

    if (count == 1) {
        if (idx == 0) {
            return ret;
        }
        return trim_copy(comment[1]);
    }

    return "";
}


bool googleBot(UserAgent& p) {
    if (contains(p.ua, "Googlebot")) {
        p.platform = "";
        p.undecided = true;
    }
    return p.undecided;
}


void setSimple(UserAgent& p, string name, string version, bool bot) {
    p.bot = bot;
    if(!bot) {
        p.mozilla = "";
    }
    p.browser.Name = name;
    p.browser.Version = version;
    p.browser.Engine = "";
    p.browser.EngineVersion = "";
    p.os = "";
    p.localization = "";
}


void fixOther(UserAgent& p, vector<Section>& sections) {
    if (sections.size() > 0) {
        p.browser.Name = sections[0].name;
        p.browser.Version = sections[0].version;
        p.mozilla = "";
    }
}


void checkBot(UserAgent& p, vector<Section>& sections) {
    size_t slen = sections.size();

    if (slen == 1 && sections[0].name != "Mozilla") {
        p.mozilla = "";
        if (boost::regex_match(sections[0].name, botRegexp)) {
            setSimple(p, sections[0].name, "", true);
            return;
        }

        string name = getFromSite(sections[0].comment);
        if (name != "") {
            setSimple(p, sections[0].name, sections[0].version, true);
            return;
        }

        setSimple(p, sections[0].name, sections[0].version, false);
    } else {
        for (size_t i = 0; i < slen; i++) {
            string name = getFromSite(sections[i].comment);
            if (name != "") {
                vector<string> s;
                split(s, name, is_any_of("/"));
                string version = "";
                if (s.size() == 2) {
                    version = s[1];
                }
                setSimple(p, s[0], version, true);
                return;
            }
        }
        fixOther(p, sections);
    }
}
