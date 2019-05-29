#ifndef SENTRY_PATH_HPP_INCLUDED
#define SENTRY_PATH_HPP_INCLUDED
#include <sys/stat.h>
#include <cctype>
#include <cstdio>
#include <string>
#include "internal.hpp"

#ifdef _WIN32
#include <shlobj_core.h>
#include <windows.h>
#endif

namespace sentry {

class Path {
   public:
#ifdef _WIN32
    Path() : Path(L"") {
    }

    Path(const wchar_t *path) : m_path(path) {
    }

    Path(const char *path);
    Path join(const wchar_t *other) const;

    const wchar_t *as_osstr() const {
        return m_path.c_str();
    }
#else
    Path() : m_path("") {
    }

    Path(const char *path) : m_path(path) {
    }

    const char *as_osstr() const {
        return m_path.c_str();
    }

#endif

    Path join(const char *other) const;
    bool create_directories();
    FILE *open(const char *mode) const;

   private:
#ifdef _WIN32
    std::wstring m_path;
#else
    std::string m_path;
#endif
};

}  // namespace sentry

#endif
