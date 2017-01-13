# user_agent
http user agent parser for c/c++


## Usage

~~~ c++
#include "userAgent.h"


int main(int argc, char **argv) {
    int opt;
    string str = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) "
        "AppleWebKit/537.36 (KHTML, like Gecko)"
        " Chrome/43.0.2357.130 Safari/537.36";

    UserAgent ua;
    Parse(ua, p.ua, p.ua.size());
    echo_ua(ua);
    
    return 0;
}
~~~

Copyright &copy; 2016-2017 yayun.tian@163.com
