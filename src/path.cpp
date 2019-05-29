#include "path.hpp"

#ifdef _WIN32

#if defined(SENTRY_BREAKPAD)
#include "common/string_conversion.h"

static std::wstring cstr_to_wtr(const char *s) {
    std::vector<uint16_t> vec;
    google_breakpad::UTF8ToUTF16(s, vec);
    return std::wstring(vec.cbegin(), vec.cend());
}

#elif defined(SENTRY_CRASHPAD)
#include "base/strings/utf_string_conversions.h"

static std::wstring cstr_to_wstr(const char *s) {
    std::wstring rv;
    base::UTF8ToUTF16(s, strlen(s), &rv);
    return rv;
}
#endif
#endif

namespace sentry {
#ifdef _WIN32
Path::Path(const char *path) : m_path(cstr_to_wstr(path)) {
}

Path Path::join(const wchar_t *other) const {
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

Path Path::join(const char *other) const {
    std::wstring tmp = cstr_to_wstr(other);
    return join(tmp.c_str());
}

bool Path::create_directories() {
    int rv = SHCreateDirectoryExW(nullptr, m_path.c_str(), nullptr);
    if (rv == ERROR_SUCCESS || rv == ERROR_ALREADY_EXISTS ||
        ERROR_FILE_EXISTS) {
        return true;
    } else {
        return false;
    }
}

FILE *Path::open(const char *mode) const {
    std::wstring wmode(mode, mode + strlen(mode));
    return _wfopen(m_path.c_str(), wmode.c_str());
}
#else
Path Path::join(const char *other) const {
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

bool Path::create_directories() {
    bool success = true;
#define _TRY_MAKE_DIR                     \
    do {                                  \
        rv = mkdir(p, 0700);              \
        if (rv != 0 && errno != EEXIST) { \
            success = false;              \
            goto done;                    \
        }                                 \
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

FILE *Path::open(const char *mode) const {
    return fopen(m_path.c_str(), mode);
}
#endif
}  // namespace sentry
