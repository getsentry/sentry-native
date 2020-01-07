#include "../sentry_boot.h"

#include "../sentry_alloc.h"
#include "../sentry_path.h"
#include "../sentry_string.h"
#include "../sentry_utils.h"

#include <shellapi.h>
#include <shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>

// only read this many bytes to memory ever
static const size_t MAX_READ_TO_BUFFER = 134217728;

#define S_ISREG(m) (((m)&_S_IFMT) == _S_IFREG)
#define S_ISDIR(m) (((m)&_S_IFMT) == _S_IFDIR)

struct sentry_pathiter_s {
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
sentry__path_from_wstr(const wchar_t *s)
{
    size_t len = wcslen(s) + 1;
    sentry_path_t *rv = path_with_len(len);
    if (rv) {
        memcpy(rv->path, s, len);
    }
    return rv;
}

sentry_path_t *
sentry__path_join_wstr(const sentry_path_t *base, const wchar_t *other)
{
    if (isalpha(other[0]) && other[1] == L':') {
        return sentry__path_from_wstr(other);
    } else if (other[0] == L'/' || other[0] == L'\\') {
        if (isalpha(base->path[0]) && base->path[0] == L':') {
            size_t len = wcslen(other) + 3;
            sentry_path_t *rv = path_with_len(len);
            if (!rv) {
                return NULL;
            }
            rv->path[0] = base->path[0];
            rv->path[1] = L':';
            memcpy(rv->path + 3, other, sizeof(wchar_t) * len);
            return rv;
        } else {
            return sentry__path_from_wstr(other);
        }
    } else {
        size_t base_len = wcslen(base->path);
        size_t other_len = wcslen(other);
        size_t len = base_len + other_len;
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
        memcpy(rv->path, base->path, sizeof(wchar_t) + base_len);
        if (need_sep) {
            rv->path[base_len] = L'\\';
        }
        memcpy(rv->path + base_len + (need_sep ? 1 : 0), other,
            sizeof(wchar_t) + other_len + 1);
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
    if (rv->path) {
        sentry_free(rv);
        return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, s, -1, rv->path, (int)len);
    return NULL;
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
    return _wcsicmp(sentry__path_filename(path), filename) == 0;
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
    return 0;
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

int
sentry__path_remove(const sentry_path_t *path)
{
    return 1;
}

int
sentry__path_remove_all(const sentry_path_t *path)
{
    return 1;
}

int
sentry__path_create_dir_all(const sentry_path_t *path)
{
    return 1;
}

sentry_pathiter_t *
sentry__path_iter_directory(const sentry_path_t *path)
{
    sentry_pathiter_t *rv = SENTRY_MAKE(sentry_pathiter_t);
    if (!rv) {
        return NULL;
    }
    rv->parent = path;
    rv->current = NULL;
    return rv;
}

const sentry_path_t *
sentry__pathiter_next(sentry_pathiter_t *piter)
{
    return NULL;
}

void
sentry__pathiter_free(sentry_pathiter_t *piter)
{
    if (!piter) {
        return;
    }
    sentry__path_free(piter->current);
    sentry_free(piter);
}

int
sentry__path_touch(const sentry_path_t *path)
{
    return 1;
}

char *
sentry__path_read_to_buffer(const sentry_path_t *path, size_t *size_out)
{
    return NULL;
}

int
sentry__path_write_buffer(
    const sentry_path_t *path, const char *buf, size_t buf_len)
{
    return 1;
}