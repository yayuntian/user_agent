//
// Created by tyy on 2017/1/5.
//

#ifndef MAFIA_UA_H
#define MAFIA_UA_H

#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>


using namespace std;
using namespace boost::algorithm;

// A section contains the name of the product, its version and
// an optional comment.
struct Section {
    string name;
    string version;
    vector<string> comment;
};

// A struct containing all the information that we might be
// interested from the browser.
struct Browser {
    string Engine;
    string EngineVersion;
    string Name;
    string Version;
};

struct OSInfo {
    string FUllName;
    string Name;
    string Version;
};

// The UserAgent struct contains all the info that can be extracted
// from the User-Agent string.
struct UserAgent {
    string ua;
    string mozilla;
    string platform;
    string os;
    string localization;

    Browser browser;
    bool bot;
    bool mobile;
    bool undecided;
};

bool googleBot(UserAgent& p);
void detectOS(UserAgent& p, Section& s, int slen);
void checkBot(UserAgent& p, vector<Section>& sections, int slen);
void detectBrowser(UserAgent& p, vector<Section>& sections, int slen);

void Parse(UserAgent& p, string ua, const int ua_len);
void echo_ua(UserAgent& p);


static inline int my_atoi(const char *buf) {
    int i = 0;
    while (!isdigit(buf[i])) i++;
    return atoi(buf + i);
}

static inline void remove_rchar(char *buf) {
    // remove json '\' character, ex: Mozilla\/5.0
    int len = strlen(buf);
    while (buf[--len] == '\\') {
        buf[len] = '\0';
    }
}
#endif //MAFIA_UA_H
