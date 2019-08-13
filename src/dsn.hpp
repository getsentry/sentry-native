#ifndef SENTRY_DSN_HPP_INCLUDED
#define SENTRY_DSN_HPP_INCLUDED
#include <string>
#include "internal.hpp"

namespace sentry {
class Dsn {
   public:
    Dsn() : m_valid(false) {
    }
    Dsn(const char *dsn);

    const char *scheme() const {
        return m_https ? "https" : "http";
    }
    const char *public_key() const {
        return m_public_key.c_str();
    }
    const char *private_key() const {
        return m_private_key.c_str();
    }
    const char *host() const {
        return m_host.c_str();
    }
    int port() const {
        return m_port;
    }
    const char *path() const {
        return m_path.c_str();
    }
    const char *project_id() const {
        return m_project_id.c_str();
    }
    bool valid() const {
        return m_valid;
    }
    bool disabled() const {
        return m_raw.empty();
    }
    const char *raw() const {
        return m_valid ? m_raw.c_str() : nullptr;
    }

    const char *get_minidump_url() const {
        return m_minidump_url.c_str();
    }
    const char *get_store_url() const {
        return m_store_url.c_str();
    }
    const char *get_auth_header() const {
        return m_auth_header.c_str();
    }

   private:
    std::string m_raw;
    bool m_https;
    std::string m_public_key;
    std::string m_private_key;
    std::string m_host;
    int m_port;
    std::string m_path;
    std::string m_project_id;
    bool m_valid;

    std::string m_minidump_url;
    std::string m_store_url;
    std::string m_auth_header;
};
}  // namespace sentry

#endif
