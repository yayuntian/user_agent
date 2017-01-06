//
// Created by tyy on 2017/1/5.
//

#ifndef MAFIA_UA_H
#define MAFIA_UA_H


using namespace std;

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
void detectOS(UserAgent& p, Section& s);
void checkBot(UserAgent& p, vector<Section> sections);
#endif //MAFIA_UA_H
