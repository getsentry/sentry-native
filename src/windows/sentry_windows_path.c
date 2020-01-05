#include "../sentry_alloc.h"
#include "../sentry_path.h"
#include "../sentry_string.h"
#include "../sentry_utils.h"

// only read this many bytes to memory ever
static const size_t MAX_READ_TO_BUFFER = 134217728;

struct sentry_pathiter_s {
    const sentry_path_t *parent;
    sentry_path_t *current;
};

sentry_path_t *
sentry__path_from_wstr(const wchar_t *s)
{
    return NULL;
}

sentry_path_t *
sentry__path_join_wstr(const sentry_path_t *base, const wchar_t *other)
{
    return NULL;
}

sentry_path_t *
sentry__path_from_str(const char *s)
{
    return NULL;
}

sentry_path_t *
sentry__path_from_str_owned(char *s)
{
    return NULL;
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
    return false;
}

bool
sentry__path_is_file(const sentry_path_t *path)
{
    return false;
}

size_t
sentry__path_get_size(const sentry_path_t *path)
{
    return 0;
}

sentry_path_t *
sentry__path_join_str(const sentry_path_t *base, const char *other)
{
    return NULL;
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