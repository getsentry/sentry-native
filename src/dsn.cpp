#include "dsn.hpp"
#include <cstring>
#include <sstream>
#include "url.hpp"

namespace sentry {

Dsn::Dsn(const char *dsn)
    : m_raw(dsn ? dsn : ""),
      m_https(false),
      m_public_key(""),
      m_private_key(""),
      m_host(""),
      m_path(""),
      m_project_id(""),
      m_valid(false) {
    // the disabled dsn
    if (m_raw.empty()) {
        m_valid = true;
        return;
    }

    Url url(dsn);
    if (!url.valid()) {
        return;
    }

    if (strcmp(url.scheme(), "https") == 0) {
        m_https = true;
    } else if (strcmp(url.scheme(), "http") == 0) {
        m_https = false;
    } else {
        return;
    }

    m_public_key = url.username();
    m_private_key = url.password();
    m_host = url.host();
    m_port = url.port();

    const char *end = strrchr(url.path(), '/');
    if (!end) {
        m_project_id = std::string(url.path());
    } else {
        m_path = std::string(url.path(), end + 1);
        m_project_id = std::string(end + 1);
    }
    m_valid = true;
}

std::string Dsn::get_minidump_url() const {
    std::stringstream ss;
    ss << scheme() << "://" << host() << ":" << port() << "/" << path()
       << "api/" << project_id() << "/minidump/?sentry_key=" << public_key();
    return ss.str();
}
}  // namespace sentry
