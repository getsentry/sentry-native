#include "path.hpp"
#include "internal.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <codecvt>
#include <locale>
#define stat_func _wstat
#define STAT _stat
#define S_ISREG(m) (((m)&_S_IFMT) == _S_IFREG)
#define S_ISDIR(m) (((m)&_S_IFMT) == _S_IFDIR)

static std::wstring cstr_to_wstr(const char *s) {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>{}
        .from_bytes(s);
}
#else
#include <unistd.h>
#define stat_func stat
#define STAT stat
#endif

namespace sentry {
#ifdef _WIN32
PathIterator::PathIterator(const Path *path)
    : m_parent(*path), m_dir_handle(nullptr) {
}

bool PathIterator::next() {
    WIN32_FIND_DATAW data;

    while (true) {
        if (!m_dir_handle) {
            std::wstring pattern = m_parent.as_osstr();
            pattern += L"\\*";
            m_dir_handle = FindFirstFileW(pattern.c_str(), &data);
            if (!m_dir_handle) {
                return false;
            }
        } else {
            if (!FindNextFileW(m_dir_handle, &data)) {
                return false;
            }
        }
        if (wcscmp(data.cFileName, L".") == 0 ||
            wcscmp(data.cFileName, L"..") == 0) {
            continue;
        } else {
            break;
        }
    }

    m_current = m_parent.join(data.cFileName);
    return true;
}

PathIterator::~PathIterator() {
    if (m_dir_handle) {
        FindClose(m_dir_handle);
    }
}

Path::Path(const char *path) : m_path(cstr_to_wstr(path)) {
}

bool Path::remove() const {
    if (!is_dir()) {
        if (DeleteFileW(m_path.c_str())) {
            return true;
        }
        return GetLastError() == ERROR_FILE_NOT_FOUND;
    } else {
        if (RemoveDirectoryW(m_path.c_str())) {
            return true;
        }
        return false;
    }
    return false;
}

bool Path::remove_all() const {
    if (!this->is_dir()) {
        return this->remove();
    }
    std::wstring path(m_path);
    path.push_back('\0');
    SHFILEOPSTRUCTW shfo = {nullptr,
                            FO_DELETE,
                            path.c_str(),
                            L"",
                            FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION,
                            false,
                            0,
                            L""};
    int rv = SHFileOperationW(&shfo);
    return rv == 0 || rv == ERROR_FILE_NOT_FOUND;
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

bool Path::create_directories() const {
    wchar_t abs_path[_MAX_PATH];
    if (_wfullpath(abs_path, m_path.c_str(), _MAX_PATH) == nullptr) {
        return false;
    }

    int rv = SHCreateDirectoryExW(nullptr, abs_path, nullptr);
    return rv == ERROR_SUCCESS || rv == ERROR_ALREADY_EXISTS ||
           rv == ERROR_FILE_EXISTS;
}

FILE *Path::open(const char *mode) const {
    std::wstring wmode(mode, mode + strlen(mode));
    return _wfopen(m_path.c_str(), wmode.c_str());
}

bool Path::filename_matches(const char *other) const {
    const wchar_t *ptr = wcsrchr(m_path.c_str(), L'/');
    if (!ptr) {
        ptr = wcsrchr(m_path.c_str(), L'\\');
    }
    if (!ptr) {
        ptr = m_path.c_str();
    } else {
        ptr++;
    }
    std::wstring wother = cstr_to_wstr(other);
    return wcscmp(ptr, wother.c_str()) == 0;
}
#else
PathIterator::PathIterator(const Path *path) {
    m_parent = *path;
    m_dir_handle = opendir(path->as_osstr());
}

bool PathIterator::next() {
    if (!m_dir_handle) {
        return false;
    }

    struct dirent *entry;
    while (true) {
        entry = readdir(m_dir_handle);
        if (entry == nullptr) {
            return false;
        }
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        break;
    }
    m_current = m_parent.join(entry->d_name);
    return true;
}

PathIterator::~PathIterator() {
    closedir(m_dir_handle);
}

bool Path::remove() const {
    int status;
    if (!is_dir()) {
        EINTR_RETRY(unlink(m_path.c_str()), &status);
        if (status == 0) {
            return true;
        }
    } else {
        EINTR_RETRY(rmdir(m_path.c_str()), &status);
        if (status == 0) {
            return true;
        }
    }
    if (errno == ENOENT) {
        return true;
    }
    return false;
}

bool Path::remove_all() const {
    if (this->is_dir()) {
        PathIterator iter = this->iter_directory();
        while (iter.next()) {
            iter.path()->remove_all();
        }
    }
    return this->remove();
}

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

bool Path::create_directories() const {
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

bool Path::filename_matches(const char *other) const {
    const char *ptr = strrchr(m_path.c_str(), '/');
    return strcmp(ptr ? ptr + 1 : m_path.c_str(), other) == 0;
}
#endif

bool Path::is_dir() const {
    struct STAT buf;
    return stat_func(m_path.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode);
}

bool Path::is_file() const {
    struct STAT buf;
    return stat_func(m_path.c_str(), &buf) == 0 && S_ISREG(buf.st_mode);
}

PathIterator Path::iter_directory() const {
    return PathIterator(this);
}
}  // namespace sentry
