#ifndef SENTRY_URL_HPP_INCLUDED
#define SENTRY_URL_HPP_INCLUDED
#include <string>

#include "internal.hpp"

namespace sentry {
class Url {
   public:
    Url() : m_valid(false) {
    }
    Url(const char *url);

    bool valid() const {
        return m_valid;
    }
    const char *scheme() const {
        return m_scheme.c_str();
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
    const char *query() const {
        return m_query.c_str();
    }
    const char *fragment() const {
        return m_fragment.c_str();
    }
    const char *username() const {
        return m_username.c_str();
    }
    const char *password() const {
        return m_password.c_str();
    }

   private:
    std::string m_scheme;
    std::string m_host;
    int m_port;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    std::string m_username;
    std::string m_password;
    bool m_valid;
};

}  // namespace sentry

#endif
