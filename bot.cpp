//
// Created by tyy on 2017/1/5.
//

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <iostream>

#include "ua.h"

using namespace boost::algorithm;


boost::regex botFromSiteRegexp{"http://.+\\.\\w+"};


string getFromSite(vector<string> comment) {
    if (comment.size() == 0) {
        return "";
    }

    int idx = (comment.size() < 3) ? 0 : 2;
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