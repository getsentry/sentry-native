#include "url.hpp"
#include <stdlib.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#define SKIP_WHILE_NOT(ptr, c) \
    while (*ptr && *ptr != c) tmp++;
#define SKIP_WHILE_NOT2(ptr, c1, c2) \
    while (*ptr && *ptr != c1 && *ptr != c2) tmp++;

static bool is_scheme_valid(const std::string &scheme_name) {
    for (char c : scheme_name) {
        if (!isalpha(c) && c != '+' && c != '-' && c != '.') {
            return false;
        }
    }

    return true;
}

namespace sentry {
Url::Url(const char *url) : m_valid(false), m_port(0) {
    // scheme
    const char *tmp = strchr(url, ':');

    if (!tmp) {
        return;
    }

    m_scheme = std::string(url, tmp - url);

    if (!is_scheme_valid(m_scheme)) {
        return;
    }

    std::transform(m_scheme.begin(), m_scheme.end(), m_scheme.begin(),
                   ::tolower);

    url = tmp + 1;

    // scheme trailer
    if (*url++ != '/') return;
    if (*url++ != '/') return;

    // auth
    bool has_username = false;
    tmp = url;
    while (*tmp) {
        if (*tmp == '@') {
            has_username = true;
            break;
        } else if (*tmp == '/') {
            has_username = false;
            break;
        }
        tmp++;
    }
    tmp = url;
    if (has_username) {
        SKIP_WHILE_NOT(tmp, '@');
        m_username = std::string(url, tmp - url);
        url = tmp;
        if (*url == ':') {
            url++;
            tmp = url;
            SKIP_WHILE_NOT(tmp, '@');
            m_password = std::string(url, tmp - url);
            url = tmp;
        }
        if (*url != '@') {
            return;
        }
        url++;
    }

    // host
    bool is_ipv6 = (*url == '[');
    tmp = url;
    while (*tmp) {
        if (is_ipv6 && *tmp == ']') {
            tmp++;
            break;
        } else if (!is_ipv6 && (*tmp == ':' || *tmp == '/')) {
            break;
        }

        tmp++;
    }

    m_host = std::string(url, tmp - url);

    // port
    url = tmp;
    if (*url == ':') {
        url++;
        tmp = url;
        SKIP_WHILE_NOT(tmp, '/');
        std::string port(url, tmp - url);
        char *end;
        m_port = strtol(port.c_str(), &end, 10);
        if (end != port.c_str() + port.size()) {
            return;
        }
        url = tmp;
    }

    if (!*url) {
        return;
    }

    // end of netloc
    if (*url != '/') {
        return;
    }

    // path
    url++;
    tmp = url;
    SKIP_WHILE_NOT2(tmp, '#', '?');
    m_path = std::string(url, tmp - url);
    url = tmp;

    // query
    if (*url == '?') {
        url++;
        tmp = url;
        SKIP_WHILE_NOT(tmp, '#');
        m_query = std::string(url, tmp - url);
        url = tmp;
    }

    // fragment
    if (*url == '#') {
        url++;
        tmp = url;
        SKIP_WHILE_NOT(tmp, 0);
        m_fragment = std::string(url, tmp - url);
        url = tmp;
    }

    if (m_port == 0) {
        if (m_scheme == "https") {
            m_port = 443;
        } else if (m_scheme == "http") {
            m_port = 80;
        }
    }

    m_valid = true;
}
}  // namespace sentry
