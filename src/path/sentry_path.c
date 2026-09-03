#include "sentry_path.h"
#include "sentry_alloc.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

sentry_path_t *
sentry__path_from_str_n(const char *s, size_t s_len)
{
    char *path = sentry__string_clone_n(s, s_len);
    if (!path) {
        return NULL;
    }
    // NOTE: function will free `path` on error
    return sentry__path_from_str_owned(path);
}

sentry_path_t *
sentry__path_from_str(const char *s)
{
    return s ? sentry__path_from_str_n(s, strlen(s)) : NULL;
}

sentry_path_t *
sentry__path_from_str_owned(char *s)
{
    sentry_path_t *rv = SENTRY_MAKE(sentry_path_t);
    if (!rv) {
        sentry_free(s);
        return NULL;
    }
    rv->path = s;
#ifdef SENTRY_PLATFORM_WINDOWS
    rv->path_w = sentry__string_to_wstr(s);
    if (!rv->path_w) {
        sentry__path_free(rv);
        return NULL;
    }
#endif
    return rv;
}

void
sentry__path_free(sentry_path_t *path)
{
    if (!path) {
        return;
    }
#ifdef SENTRY_PLATFORM_WINDOWS
    sentry_free(path->path_w);
#endif
    sentry_free(path->path);
    sentry_free(path);
}

bool
sentry__path_eq(const sentry_path_t *path_a, const sentry_path_t *path_b)
{
    size_t i = 0;
    while (path_a->path[i] == path_b->path[i]) {
        if (path_a->path[i] == (char)0) {
            return true;
        }
        i++;
    }
    return false;
}

int
sentry__path_remove_all(const sentry_path_t *path)
{
    if (sentry__path_is_dir(path) && !sentry__path_is_symlink(path)) {
        sentry_pathiter_t *piter = sentry__path_iter_directory(path);
        if (piter) {
            const sentry_path_t *p;
            while ((p = sentry__pathiter_next(piter)) != NULL) {
                sentry__path_remove_all(p);
            }
            sentry__pathiter_free(piter);
        }
    }
    return sentry__path_remove(path);
}

sentry_path_t *
sentry__path_unique(const sentry_path_t *dir, const char *basename)
{
    if (!dir || sentry__string_empty(basename)) {
        return NULL;
    }

    const char *dot = strrchr(basename, '.');
    bool has_ext = dot && dot != basename;
    size_t stem_len = has_ext ? (size_t)(dot - basename) : strlen(basename);
    const char *ext = has_ext ? dot : "";

    char buf[512];
    for (int n = 0; n < 10000; n++) {
        if (n == 0) {
            snprintf(buf, sizeof(buf), "%s", basename);
        } else {
            snprintf(
                buf, sizeof(buf), "%.*s-%d%s", (int)stem_len, basename, n, ext);
        }
        sentry_path_t *candidate = sentry__path_join_str(dir, buf);
        if (!candidate || !sentry__path_is_file(candidate)) {
            return candidate;
        }
        sentry__path_free(candidate);
    }
    return NULL;
}

sentry_filelock_t *
sentry__filelock_new(sentry_path_t *path)
{
    sentry_filelock_t *rv = SENTRY_MAKE(sentry_filelock_t);
    if (!rv) {
        sentry__path_free(path);
        return NULL;
    }
    rv->path = path;
    rv->is_locked = false;

    return rv;
}

void
sentry__filelock_free(sentry_filelock_t *lock)
{
    sentry__filelock_unlock(lock);
    sentry__path_free(lock->path);
    sentry_free(lock);
}

sentry_path_t *
sentry__path_basename(const sentry_path_t *path, const char *suffix)
{
    size_t path_len = strlen(path->path);
    if (suffix) {
        size_t suffix_len = strlen(suffix);
        if (suffix_len > 0 && path_len > suffix_len
            && strcmp(path->path + path_len - suffix_len, suffix) == 0) {
            return sentry__path_from_str_n(path->path, path_len - suffix_len);
        }
    } else {
        const char *filename = sentry__path_filename(path);
        const char *dot = strrchr(filename, '.');
        if (dot && dot != filename) {
            return sentry__path_from_str_n(
                path->path, (size_t)(dot - path->path));
        }
    }
    return NULL;
}

FILE *
sentry__path_open(const sentry_path_t *path, const char *mode, size_t offset)
{
    if (!path || !mode) {
        return NULL;
    }
#ifdef SENTRY_PLATFORM_WINDOWS
    wchar_t *mode_w = sentry__string_to_wstr(mode);
    FILE *file = mode_w ? _wfopen(path->path_w, mode_w) : NULL;
    sentry_free(mode_w);
#else
    FILE *file = fopen(path->path, mode);
#endif
    if (!file) {
        return NULL;
    }
#ifdef SENTRY_PLATFORM_WINDOWS
    int result
        = offset > INT64_MAX ? -1 : _fseeki64(file, (__int64)offset, SEEK_SET);
#elif defined(SENTRY_PLATFORM_NX)
    int result = offset > LONG_MAX ? -1 : fseek(file, (long)offset, SEEK_SET);
#else
    off_t file_offset = (off_t)offset;
    int result = file_offset < 0 || (size_t)file_offset != offset
        ? -1
        : fseeko(file, file_offset, SEEK_SET);
#endif
    if (result != 0) {
        fclose(file);
        return NULL;
    }
    return file;
}
