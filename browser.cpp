//
// Created by tyy on 2017/1/6.
//

#include <iostream>
#include <cstdlib>
#include <vector>
#include <unordered_map>

#include "userAgent.h"


boost::regex ie11Regexp{"^rv:(.+)$"};

void detectBrowser(UserAgent& p, vector<Section>& sections, int slen) {

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
                } else if (name == "QQBrowser") {
                    p.browser.Name = name;
                    p.browser.Version = sections[slen - 1].version;
                } else if (name == "360SE") {
                    p.browser.Name = name;
//                    p.browser.Version = sections[slen - 1].version;
                } else if (name == "LBBROWSER") {
                    p.browser.Name = name;
//                    p.browser.Version = sections[slen - 1].version;
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
                        p.browser.Version = what[1];
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

            if (starts_with(comment[comment.size() - 1], "360SE")) {
                p.browser.Name = "360SE";
            }

            size_t len = comment.size();
            for (size_t i = 0; i < len; i++) {
                if (starts_with(comment[i], "Trident")) {
                    switch (my_atoi(comment[i].c_str())) {
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
                string ver;
                ver.assign(comment[1], 4, string::npos);
                p.browser.Version = trim_copy(ver);
            }
        }
    }
}

