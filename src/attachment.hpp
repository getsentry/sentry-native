#ifndef SENTRY_ATTACHMENT_HPP_INCLUDED
#define SENTRY_ATTACHMENT_HPP_INCLUDED
#include <string>

#include "internal.hpp"
#include "path.hpp"

namespace sentry {

class Attachment {
   public:
    Attachment(const char *name, Path path) : m_name(name), m_path(path) {
    }

    const char *name() const {
        return m_name.c_str();
    }
    const sentry::Path &path() const {
        return m_path;
    }

   private:
    std::string m_name;
    sentry::Path m_path;
};

}  // namespace sentry

#endif
