#ifndef SENTRY_PATH_HPP_INCLUDED
#define SENTRY_PATH_HPP_INCLUDED
#include <sys/stat.h>
#include <cctype>
#include <cstdio>
#include <string>
#include "internal.hpp"

#ifdef _WIN32
#if defined(SENTRY_BREAKPAD)
#include "common/string_conversion.h"

std::wstring cstr_to_wtr(const char *s) {
    std::vector<uint16_t> vec;
    google_breakpad::UTF8ToUTF16(s, vec);
    return std::wstring(vec.cbegin(), vec.cend());
}

#elif defined(SENTRY_CRASHPAD)
#include "mini_chromium/base/strings/utf_string_conversions.h"

std::wstring cstr_to_wstr(const char *s) {
    std::wstring rv;
    base::UTF8ToUTF16(s, strlen(s), rv);
    return rv;
}
#endif
#endif

namespace sentry {

class Path {
   public:
#ifdef _WIN32
    Path() : Path(L"") {
    }

    Path(const wchar_t *path) : m_path(path) {
    }

    Path(const char *path) : m_path(cstr_to_wstr(path)) {
    }

    Path join(const char *other) const {
        std::wstring tmp = cstr_to_wstr(other);
        return join(tmp.c_str());
    }

    Path join(const wchar_t *other) const {
        if (::isalpha(*other) && other[1] == L':') {
            return Path(other);
        } else {
            Path rv = Path(m_path.c_str());
            if (m_path[m_path.size() - 1] != L'/' &&
                m_path[m_path.size() - 1] != L'\\') {
                rv.m_path.push_back('\\');
            }
            rv.m_path.append(other);
            return rv;
        }
    }
#else
    Path() : m_path("") {
    }

    Path(const char *path) : m_path(path) {
    }

    Path join(const char *other) const {
        if (*other == '/') {
            return Path(other);
        } else {
            Path rv = Path(m_path.c_str());
            if (m_path.empty() || m_path[m_path.size() - 1] != '/') {
                rv.m_path.push_back('/');
            }
            rv.m_path.append(other);
            return rv;
        }
    }

    bool create_directories() {
        bool success = true;
#define _TRY_MAKE_DIR                                                         \
    do {                                                                      \
        rv = mkdir(p, 0700);                                                  \
        if (rv != 0 && errno != EEXIST) {                                     \
            SENTRY_PRINT_ERROR_ARGS("Failed to create directory '%s'", path); \
            success = false;                                                  \
            goto done;                                                        \
        }                                                                     \
    } while (0)

        const char *path = m_path.c_str();
        int rv = 0;
        char *p = strdup(path);
        char *ptr;
        for (ptr = p; *ptr; ptr++) {
            if (*ptr == '/') {
                *ptr = 0;
                _TRY_MAKE_DIR;
                *ptr = '/';
            }
        }
        _TRY_MAKE_DIR;
#undef _TRY_MAKE_DIR
    done:
        free(p);
        return success;
    }

    FILE *open(const char *mode) const {
        return fopen(m_path.c_str(), mode);
    }

    const char *as_cstr() const {
        return m_path.c_str();
    }

    const char *as_osstr() const {
        return m_path.c_str();
    }

#endif

   private:
#ifdef _WIN32
    std::wstring m_path;
    // XXX: cstr
#else
    std::string m_path;
#endif
};  // namespace sentry

}  // namespace sentry

#endif
