#include "sentry_boot.h"

#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_utils.h"

#if _WIN32_WINNT >= 0x0602
#    include <pathcch.h>
#endif
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <sys/stat.h>
#include <sys/types.h>

// only read this many bytes to memory ever
static const size_t MAX_READ_TO_BUFFER = 134217728;

#ifndef __MINGW32__
#    define S_ISREG(m) (((m)&_S_IFMT) == _S_IFREG)
#    define S_ISDIR(m) (((m)&_S_IFMT) == _S_IFDIR)
#endif

struct sentry_pathiter_s {
    HANDLE dir_handle;
    const sentry_path_t *parent;
    sentry_path_t *current;
};

static sentry_path_t *
path_with_len(size_t len)
{
    sentry_path_t *rv = SENTRY_MAKE(sentry_path_t);
    rv->path = sentry_malloc(sizeof(wchar_t) * len);
    if (!rv->path) {
        sentry_free(rv);
        return NULL;
    }
    return rv;
}

sentry_path_t *
sentry__path_current_exe(void)
{
    // inspired by:
    // https://github.com/rust-lang/rust/blob/183e893aaae581bd0ab499ba56b6c5e118557dc7/src/libstd/sys/windows/os.rs#L234-L239
    sentry_path_t *path = path_with_len(MAX_PATH);
    size_t len = GetModuleFileNameW(NULL, path->path, MAX_PATH);
    if (!len) {
        SENTRY_DEBUG("unable to get current exe path");
        sentry__path_free(path);
        return NULL;
    }
    return path;
}

sentry_path_t *
sentry__path_dir(const sentry_path_t *path)
{
    sentry_path_t *dir_path = sentry__path_clone(path);
    if (!dir_path) {
        return NULL;
    }
#if _WIN32_WINNT >= 0x0602 && !defined(__MINGW32__)
    PathCchRemoveFileSpec(dir_path->path, wcslen(dir_path->path));
#else
    PathRemoveFileSpecW(dir_path->path);
#endif
    return dir_path;
}

sentry_path_t *
sentry__path_from_wstr(const wchar_t *s)
{
    size_t len = wcslen(s) + 1;
    sentry_path_t *rv = path_with_len(len);
    if (rv) {
        memcpy(rv->path, s, len * sizeof(wchar_t));
    }
    return rv;
}

sentry_path_t *
sentry__path_join_wstr(const sentry_path_t *base, const wchar_t *other)
{
    if (isalpha(other[0]) && other[1] == L':') {
        return sentry__path_from_wstr(other);
    } else if (other[0] == L'/' || other[0] == L'\\') {
        if (isalpha(base->path[0]) && base->path[1] == L':') {
            size_t len = wcslen(other) + 3;
            sentry_path_t *rv = path_with_len(len);
            if (!rv) {
                return NULL;
            }
            rv->path[0] = base->path[0];
            rv->path[1] = L':';
            memcpy(rv->path + 2, other, sizeof(wchar_t) * len);
            return rv;
        } else {
            return sentry__path_from_wstr(other);
        }
    } else {
        size_t base_len = wcslen(base->path);
        size_t other_len = wcslen(other);
        size_t len = base_len + other_len + 1;
        bool need_sep = false;
        if (base_len && base->path[base_len - 1] != L'/'
            && base->path[base_len - 1] != L'\\') {
            len += 1;
            need_sep = true;
        }
        sentry_path_t *rv = path_with_len(len);
        if (!rv) {
            return NULL;
        }
        memcpy(rv->path, base->path, sizeof(wchar_t) * base_len);
        if (need_sep) {
            rv->path[base_len] = L'\\';
        }
        memcpy(rv->path + base_len + (need_sep ? 1 : 0), other,
            sizeof(wchar_t) * (other_len + 1));
        return rv;
    }
}

sentry_path_t *
sentry__path_from_str(const char *s)
{
    size_t len = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    sentry_path_t *rv = SENTRY_MAKE(sentry_path_t);
    if (!rv) {
        return NULL;
    }
    rv->path = sentry_malloc(sizeof(wchar_t) * len);
    if (!rv->path) {
        sentry_free(rv);
        return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, s, -1, rv->path, (int)len);
    return rv;
}

sentry_path_t *
sentry__path_from_str_owned(char *s)
{
    sentry_path_t *rv = sentry__path_from_str(s);
    sentry_free(s);
    return rv;
}

const sentry_pathchar_t *
sentry__path_filename(const sentry_path_t *path)
{
    const wchar_t *s = path->path;
    const wchar_t *ptr = s;
    size_t idx = wcslen(s);

    while (true) {
        if (s[idx] == L'/' || s[idx] == '\\') {
            ptr = s + idx + 1;
            break;
        }
        if (idx > 0) {
            idx -= 1;
        } else {
            break;
        }
    }

    return ptr;
}

bool
sentry__path_filename_matches(const sentry_path_t *path, const char *filename)
{
    sentry_path_t *fn = sentry__path_from_str(filename);
    bool matches = _wcsicmp(sentry__path_filename(path), fn->path) == 0;
    sentry__path_free(fn);
    return matches;
}

bool
sentry__path_ends_with(const sentry_path_t *path, const char *suffix)
{
    sentry_path_t *s = sentry__path_from_str(suffix);
    size_t pathlen = wcslen(path->path);
    size_t suffixlen = wcslen(s->path);
    if (suffixlen > pathlen) {
        sentry__path_free(s);
        return false;
    }

    bool matches = _wcsicmp(&path->path[pathlen - suffixlen], s->path) == 0;
    sentry__path_free(s);
    return matches;
}

bool
sentry__path_is_dir(const sentry_path_t *path)
{
    struct _stat buf;
    return _wstat(path->path, &buf) == 0 && S_ISDIR(buf.st_mode);
}

bool
sentry__path_is_file(const sentry_path_t *path)
{
    struct _stat buf;
    return _wstat(path->path, &buf) == 0 && S_ISREG(buf.st_mode);
}

size_t
sentry__path_get_size(const sentry_path_t *path)
{
    struct _stat buf;
    if (_wstat(path->path, &buf) == 0 && S_ISREG(buf.st_mode)) {
        return (size_t)buf.st_size;
    } else {
        return 0;
    }
}

sentry_path_t *
sentry__path_join_str(const sentry_path_t *base, const char *other)
{
    sentry_path_t *other_path = sentry__path_from_str(other);
    if (!other_path) {
        return NULL;
    }
    sentry_path_t *rv = sentry__path_join_wstr(base, other_path->path);
    sentry__path_free(other_path);
    return rv;
}

sentry_path_t *
sentry__path_clone(const sentry_path_t *path)
{
    sentry_path_t *rv = SENTRY_MAKE(sentry_path_t);
    if (!rv) {
        return NULL;
    }
    rv->path = _wcsdup(path->path);
    return rv;
}

int
sentry__path_remove(const sentry_path_t *path)
{
    if (!sentry__path_is_dir(path)) {
        if (DeleteFileW(path->path)) {
            return 0;
        }
        return GetLastError() == ERROR_FILE_NOT_FOUND ? 0 : 1;
    } else {
        if (RemoveDirectoryW(path->path)) {
            return 0;
        }
        return 1;
    }
    return 1;
}

int
sentry__path_remove_all(const sentry_path_t *path)
{
    if (!sentry__path_is_dir(path)) {
        return sentry__path_remove(path);
    }
    size_t path_len = wcslen(path->path);
    wchar_t *pp = sentry_malloc(sizeof(wchar_t) * (path_len + 2));
    memcpy(pp, path->path, path_len * sizeof(wchar_t));
    pp[path_len] = '\0';
    pp[path_len + 1] = '\0';
    SHFILEOPSTRUCTW shfo = { NULL, FO_DELETE, pp, L"",
        FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION, FALSE, 0, L"" };
    int rv = SHFileOperationW(&shfo);
    sentry_free(pp);
    return rv == 0 || rv == ERROR_FILE_NOT_FOUND ? 0 : 1;
}

int
sentry__path_create_dir_all(const sentry_path_t *path)
{
    wchar_t abs_path[_MAX_PATH];
    if (_wfullpath(abs_path, path->path, _MAX_PATH) == NULL) {
        return 1;
    }

    int rv = SHCreateDirectoryExW(NULL, abs_path, NULL);
    if (rv == ERROR_SUCCESS || rv == ERROR_ALREADY_EXISTS
        || rv == ERROR_FILE_EXISTS) {
        return 0;
    } else {
        return 1;
    }
}

sentry_pathiter_t *
sentry__path_iter_directory(const sentry_path_t *path)
{
    sentry_pathiter_t *rv = SENTRY_MAKE(sentry_pathiter_t);
    if (!rv) {
        return NULL;
    }
    rv->dir_handle = INVALID_HANDLE_VALUE;
    rv->parent = path;
    rv->current = NULL;
    return rv;
}

const sentry_path_t *
sentry__pathiter_next(sentry_pathiter_t *piter)
{
    WIN32_FIND_DATAW data;

    while (true) {
        if (piter->dir_handle == INVALID_HANDLE_VALUE) {
            size_t path_len = wcslen(piter->parent->path);
            wchar_t *pattern = sentry_malloc(sizeof(wchar_t) * (path_len + 3));
            if (!pattern) {
                return NULL;
            }
            memcpy(pattern, piter->parent->path, sizeof(wchar_t) * path_len);
            pattern[path_len] = L'\\';
            pattern[path_len + 1] = L'*';
            pattern[path_len + 2] = 0;
            piter->dir_handle = FindFirstFileW(pattern, &data);
            if (piter->dir_handle == INVALID_HANDLE_VALUE) {
                return NULL;
            }
        } else {
            if (!FindNextFileW(piter->dir_handle, &data)) {
                return NULL;
            }
        }
        if (wcscmp(data.cFileName, L".") == 0
            || wcscmp(data.cFileName, L"..") == 0) {
            continue;
        } else {
            break;
        }
    }

    if (piter->current) {
        sentry__path_free(piter->current);
    }
    piter->current = sentry__path_join_wstr(piter->parent, data.cFileName);
    return piter->current;
}

void
sentry__pathiter_free(sentry_pathiter_t *piter)
{
    if (!piter) {
        return;
    }
    if (piter->dir_handle != INVALID_HANDLE_VALUE) {
        FindClose(piter->dir_handle);
    }
    sentry__path_free(piter->current);
    sentry_free(piter);
}

int
sentry__path_touch(const sentry_path_t *path)
{
    FILE *f = _wfopen(path->path, L"a");
    if (f) {
        fclose(f);
        return 0;
    }
    return 1;
}

char *
sentry__path_read_to_buffer(const sentry_path_t *path, size_t *size_out)
{
    FILE *f = _wfopen(path->path, L"rb");
    if (!f) {
        return NULL;
    }
    size_t len = sentry__path_get_size(path);
    if (len == 0) {
        fclose(f);
        char *rv = sentry_malloc(1);
        rv[0] = '\0';
        if (size_out) {
            *size_out = 0;
        }
        return rv;
    } else if (len > MAX_READ_TO_BUFFER) {
        fclose(f);
        return NULL;
    }

    // this is completely not sane in concurrent situations but hey
    char *rv = sentry_malloc(len + 1);
    if (!rv) {
        fclose(f);
        return NULL;
    }

    size_t remaining = len;
    size_t offset = 0;
    while (true) {
        size_t n = fread(rv + offset, 1, remaining, f);
        if (n == 0) {
            break;
        }
        offset += n;
        remaining -= n;
    }

    rv[offset] = '\0';
    fclose(f);

    if (size_out) {
        *size_out = offset;
    }
    return rv;
}

static int
write_buffer_with_mode(const sentry_path_t *path, const char *buf,
    size_t buf_len, const wchar_t *mode)
{
    FILE *f = _wfopen(path->path, mode);
    if (!f) {
        return 1;
    }

    size_t offset = 0;
    size_t remaining = buf_len;

    while (true) {
        if (remaining == 0) {
            break;
        }
        size_t n = fwrite(buf + offset, 1, remaining, f);
        if (n == 0) {
            break;
        }
        offset += n;
        remaining -= n;
    }

    fclose(f);
    return remaining == 0 ? 0 : 1;
}

int
sentry__path_write_buffer(
    const sentry_path_t *path, const char *buf, size_t buf_len)
{
    return write_buffer_with_mode(path, buf, buf_len, L"wb");
}

int
sentry__path_append_buffer(
    const sentry_path_t *path, const char *buf, size_t buf_len)
{
    return write_buffer_with_mode(path, buf, buf_len, L"a");
}
