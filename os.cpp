//
// Created by tyy on 2017/1/6.
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

#include "ua.h"

using namespace std;
using namespace boost::algorithm;

string getPlatform(vector<string>& comment) {
    if (comment.size() > 0) {
        if (comment[0] != "compatible") {
            if (starts_with(comment[0], "Windows")) {
                return "Windows";
            } else if  (starts_with(comment[0], "Symbian")) {
                return "Symbian";
            } else if (starts_with(comment[0], "webOS")) {
                return "webOS";
            } else if (comment[0] == "BB10") {
                return "BlackBerry";
            }
            return comment[0];
        }
    }
    return "";
}


string normalizeOS(string name) {
    vector<string> sp;
    split(sp, name, is_any_of(" "));

    if (sp.size() != 3 || sp[1] != "NT") {
        return name;
    }

    if (sp[2] == "5.0") {
        return "Windows 2000";
    } else if (sp[2] == "5.01") {
        return "Windows 2000, Service Pack 1 (SP1)";
    } else if (sp[2] == "5.1") {
        return "Windows XP";
    } else if (sp[2] == "5.2") {
        return "Windows XP x64 Edition";
    } else if (sp[2] == "6.0") {
        return "Windows Vista";
    } else if (sp[2] == "6.1") {
        return "Windows 7";
    } else if (sp[2] == "6.2") {
        return "Windows 8";
    } else if (sp[2] == "6.3") {
        return "Windows 8.1";
    } else if (sp[2] == "10.0") {
        return "Windows 10";
    }
    return name;
}


void webKit(UserAgent& p, vector<string>& comment) {
    if (p.platform == "webOS") {
        p.browser.Name = p.platform;
        p.os = "Palm";
        if (comment.size() > 2) {
            p.localization = comment[2];
        }
        p.mobile = true;
    } else if (p.platform == "Symbian") {
        p.mobile = true;
        p.browser.Name = p.platform;
        p.os = comment[0];
    } else if (p.platform == "Linux") {
        p.mobile = true;
        if (p.browser.Name == "Safari") {
            p.browser.Name = "Android";
        }
        if (comment.size() > 1) {
            if (comment[1] == "U") {
                if (comment.size() > 2) {
                    p.os = comment[2];
                } else {
                    p.mobile = false;
                    p.os = comment[0];
                }
            } else {
                p.os = comment[1];
            }
        }
        if (comment.size() > 3) {
            p.localization = comment[3];
        }
    } else if (comment.size() > 0) {
        if (comment.size() > 3) {
            p.localization = comment[3];
        }

        if (starts_with(comment[0], "Windows NT")) {
            p.os = normalizeOS(comment[0]);
        } else if (comment.size() < 2) {
            p.localization = comment[0];
        } else if (comment.size() < 3) {
            if (!googleBot(p)) {
                p.os = normalizeOS(comment[1]);
            }
        } else {
            p.os = normalizeOS(comment[2]);
        }
        if (p.platform == "BlackBerry") {
            p.browser.Name = p.platform;
            if (p.os == "Touch") {
                p.os = p.platform;
            }
        }
    }
}


void gecko(UserAgent& p, vector<string>& comment) {
    if (comment.size() <= 1) {
        return;
    }

    if (comment[1] == "U") {
        if (comment.size() > 2) {
            p.os = normalizeOS(comment[2]);
        } else {
            p.os = normalizeOS(comment[1]);
        }
    } else {
        if (p.platform == "Andriod") {
            p.mobile = true;
            p.platform = normalizeOS(comment[1]);
            p.os = p.platform;
        } else if (comment[0] == "Mobile" || comment[0] == "Tablet") {
            p.mobile = true;
            p.os = "FireFoxOS";
        } else {
            if (p.os == "") {
                p.os = normalizeOS(comment[1]);
            }
        }
    }

    if (comment.size() > 3 && !starts_with(comment[3], "rv:")) {
        p.localization = comment[3];
    }
}


void trident(UserAgent& p, vector<string>& comment) {
    p.platform = "Windows";

    if (p.os == "") {
        if (comment.size() > 2) {
            p.os = normalizeOS(comment[2]);
        } else {
            p.os = "Windows NT 4.0";
        }
    }

    for (string s : comment) {
        if (starts_with(s, "IEMobile")) {
            p.mobile = true;
            return;
        }
    }
}


void opera(UserAgent& p, vector<string>& comment) {
    int slen = comment.size();

    if (starts_with(comment[0], "Windows")) {
        p.platform = "Windows";
        p.os = normalizeOS(comment[0]);
        if (slen > 2) {
            if (slen > 3 && starts_with(comment[2], "MRA")) {
                p.localization = comment[3];
            } else {
                p.localization = comment[2];
            }
        }
    } else {
        if (starts_with(comment[0], "Android")) {
            p.mobile = true;
        }
        p.platform = comment[0];
        if (slen > 1) {
            p.os = comment[0];
            if (slen > 3) {
                p.localization = comment[3];
            }
        } else {
            p.os = comment[0];
        }
    }
}


void dalvik(UserAgent& p, vector<string>& comment) {
    int slen = comment.size();

    if (starts_with(comment[0], "Linux")) {
        p.platform = comment[0];
        if (slen > 2) {
            p.os = comment[2];
        }
        p.mobile = true;
    }
}


vector<string> split_v(const vector<string>& data, int start, int end)
{
    int n = end - start;
    if (n <= 0) {
        return std::vector<string>();
    }

    std::vector<string> y(n);
    for (int i = 0; i < n; i++) {
        y[i] = data[start++];
    }
    return y;
}


void osName(vector<string>& osSplit, string& name, string& version) {
    OSInfo osInfo;

    if (osSplit.size() == 1) {
        name = osSplit[0];
        version = "";
    } else {
        vector<string> nameSplit = split_v(osSplit, 0, osSplit.size() - 1);
        version = osSplit[osSplit.size() - 1];

        if (nameSplit.size() > 2 && nameSplit[0] == "Intel" &&
                nameSplit[1] == "Mac") {
            nameSplit = split_v(nameSplit, 1, nameSplit.size());
        }
        name = join(nameSplit, " ");

        if (contains(version, "x86") || contains(version, "i686")) {
            version = "";
        } else if (version == "X" && name == "Mac OS") {
            name = name + " " + version;
            version = "";
        }
    }
}


OSInfo getOSInfo(UserAgent& p) {

    string os = replace_first_copy(p.os, "like Mac OS X", "");
    os = replace_first_copy(os, "CPU", "");
    os = trim_copy_if(os, is_any_of(" "));

    vector<string> osSplit;
    split(osSplit, os, is_any_of(" "));

    if (os == "Windows XP x64 Edition") {
        osSplit = split_v(osSplit, 0, 1);
    }

    string name, version;
    osName(osSplit, name, version);

    if (contains(name, "/")) {
        vector<string> s;
        split(s, name, is_any_of("/"));
        name = s[0];
        version = s[1];
    }

    version = replace_all_copy(version, "_", ".");

    return OSInfo {
            .FUllName = p.os,
            .Name = name,
            .Version = version
    };

}


void detectOS(UserAgent& p, Section& s) {
    if (s.name == "Mozilla") {
        p.platform = getPlatform(s.comment);
        if (p.platform == "Windows" && s.comment.size() > 0) {
            p.os = normalizeOS(s.comment[0]);
        }

        if (p.browser.Engine == "") {
            p.undecided = true;
        } else if(p.browser.Engine == "Gecko") {
            gecko(p, s.comment);
        } else if (p.browser.Engine == "AppleWebKit") {
            webKit(p, s.comment);
        } else if (p.browser.Engine == "Trident") {
            trident(p, s.comment);
        }
    } else if (s.name == "Opera") {
        if (s.comment.size() > 0) {
            opera(p, s.comment);
        }
    } else if (s.name == "Dalvik") {
        if (s.comment.size() > 0) {
            dalvik(p, s.comment);
        }
    } else {
        p.undecided = true;
    }
}

