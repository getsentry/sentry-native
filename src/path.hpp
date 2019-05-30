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
#else
#include <dirent.h>
#include <sys/types.h>
#endif

namespace sentry {

class PathIterator;

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

    bool is_dir() const;
    bool is_file() const;
    Path join(const char *other) const;
    bool create_directories() const;
    bool remove() const;
    bool remove_all() const;
    PathIterator iter_directory() const;
    FILE *open(const char *mode) const;
    bool filename_matches(const char *other) const;

   private:
#ifdef _WIN32
    std::wstring m_path;
#else
    std::string m_path;
#endif
};

class PathIterator {
   public:
    PathIterator(const Path *path);
    ~PathIterator();
    const sentry::Path *path() const {
        return &m_current;
    }
    bool next();

#ifdef _WIN32
    HANDLE m_dir_handle;
#else
    DIR *m_dir_handle;
#endif
    Path m_parent;
    Path m_current;
};

}  // namespace sentry

#endif
