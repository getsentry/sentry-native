#include "dsn.hpp"
#include <cstring>
#include <sstream>
#include "url.hpp"
#include "uuid.hpp"

using namespace sentry;

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

    // since a DSN is immutable we can precalculate these
    std::stringstream ss;
    ss << scheme() << "://" << host() << ":" << port() << "/" << path()
       << "api/" << project_id() << "/minidump/?sentry_key=" << public_key();
    m_minidump_url = ss.str();
    ss.str("");
    ss << scheme() << "://" << host() << ":" << port() << "/" << path()
       << "api/" << project_id() << "/store/";
    m_store_url = ss.str();
    ss.str("");
    ss << "Sentry sentry_key=" << m_public_key << ", sentry_version=7, "
       << "sentry_client=" << SENTRY_SDK_USER_AGENT;
    m_auth_header = ss.str();
}

std::string Dsn::get_attachment_url(const sentry_uuid_t *event_id) const {
    char event_id_buf[40];
    sentry_uuid_as_string(event_id, event_id_buf);
    std::stringstream ss;
    ss << scheme() << "://" << host() << ":" << port() << "/" << path()
       << "api/" << project_id() << "/events/" << event_id_buf
       << "/attachments/";
    return ss.str();
}
