#include "../sentry_alloc.h"
#include "../sentry_path.h"
#include "../sentry_string.h"
#include "../sentry_utils.h"

#include <dirent.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>

struct sentry_pathiter_s {
    const sentry_path_t *parent;
    sentry_path_t *current;
    DIR *dir_handle;
};

sentry_path_t *
sentry__path_from_str(const char *s)
{
    char *path = sentry__string_dup(s);
    sentry_path_t *rv = NULL;
    if (!path) {
        return NULL;
    }
    rv = sentry__path_from_str_owned(path);
    if (rv) {
        return rv;
    }
    sentry_free(path);
    return NULL;
}

sentry_path_t *
sentry__path_from_str_owned(char *s)
{
    sentry_path_t *rv = SENTRY_MAKE(sentry_path_t);
    if (!rv) {
        return NULL;
    }
    rv->path = s;
    return rv;
}

const sentry_pathchar_t *
sentry__path_filename(const sentry_path_t *path)
{
    const char *c = strrchr(path->path, '/');
    return c ? c + 1 : NULL;
}

bool
sentry__path_filename_matches(const sentry_path_t *path, const char *filename)
{
    return strcmp(sentry__path_filename(path), filename) == 0;
}

bool
sentry__path_is_dir(const sentry_path_t *path)
{
    struct stat buf;
    return stat(path->path, &buf) == 0 && S_ISDIR(buf.st_mode);
}

bool
sentry__path_is_file(const sentry_path_t *path)
{
    struct stat buf;
    return stat(path->path, &buf) == 0 && S_ISREG(buf.st_mode);
}

sentry_path_t *
sentry__path_join_str(const sentry_path_t *base, const char *other)
{
    sentry_stringbuilder_t sb;

    if (*other == '/') {
        return sentry__path_from_str(other);
    }

    sentry__stringbuilder_init(&sb);
    sentry__stringbuilder_append(&sb, base->path);

    if (!base->path[0] || base->path[strlen(base->path) - 1] != '/') {
        sentry__stringbuilder_append_char(&sb, '/');
    }
    sentry__stringbuilder_append(&sb, other);

    return sentry__path_from_str_owned(sentry__stringbuilder_into_string(&sb));
}

int
sentry__path_remove(const sentry_path_t *path)
{
    int status;
    if (!sentry__path_is_dir(path)) {
        EINTR_RETRY(unlink(path->path), &status);
        if (status == 0) {
            return 0;
        }
    } else {
        EINTR_RETRY(rmdir(path->path), &status);
        if (status == 0) {
            return 0;
        }
    }
    if (errno == ENOENT) {
        return 0;
    }
    return 1;
}

int
sentry__path_remove_all(const sentry_path_t *path)
{
    if (sentry__path_is_dir(path)) {
        sentry_pathiter_t *piter = sentry__path_iter_directory(path);
        const sentry_path_t *p;
        while ((p = sentry__pathiter_next(piter)) != NULL) {
            sentry__path_remove_all(p);
        }
        sentry__pathiter_free(piter);
    }
    return sentry__path_remove(path);
}

int
sentry__path_create_dir_all(const sentry_path_t *path)
{
    char *p, *ptr;
    int rv = 0;
#define _TRY_MAKE_DIR                                                          \
    do {                                                                       \
        int mrv = mkdir(p, 0700);                                              \
        if (mrv != 0 && errno != EEXIST) {                                     \
            rv = 1;                                                            \
            goto done;                                                         \
        }                                                                      \
    } while (0)

    p = sentry__string_dup(path->path);
    for (ptr = p; *ptr; ptr++) {
        if (*ptr == '/' && ptr != p) {
            *ptr = 0;
            _TRY_MAKE_DIR;
            *ptr = '/';
        }
    }
    _TRY_MAKE_DIR;
#undef _TRY_MAKE_DIR

done:
    sentry_free(p);
    return rv;
}

FILE *
sentry__path_open(const sentry_path_t *path, const char *mode)
{
    return fopen(path->path, mode);
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
    rv->dir_handle = opendir(path->path);
    return rv;
}

const sentry_path_t *
sentry__pathiter_next(sentry_pathiter_t *piter)
{
    struct dirent *entry;

    if (!piter->dir_handle) {
        return NULL;
    }

    while (true) {
        entry = readdir(piter->dir_handle);
        if (!entry) {
            return NULL;
        }
        if (strcmp(entry->d_name, ".") == 0
            || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        break;
    }

    sentry__path_free(piter->current);
    piter->current = sentry__path_join_str(piter->parent, entry->d_name);

    return piter->current;
}

void
sentry__pathiter_free(sentry_pathiter_t *piter)
{
    if (!piter) {
        return;
    }
    if (piter->dir_handle) {
        closedir(piter->dir_handle);
    }
    sentry__path_free(piter->current);
    sentry_free(piter);
}