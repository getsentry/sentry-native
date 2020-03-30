#ifndef SENTRY_PATH_H_INCLUDED
#define SENTRY_PATH_H_INCLUDED

#include "sentry_boot.h"

#include <stdio.h>

#ifdef SENTRY_PLATFORM_WINDOWS
typedef wchar_t sentry_pathchar_t;
#    define SENTRY_PATH_PRI "S"
#else
typedef char sentry_pathchar_t;
#    define SENTRY_PATH_PRI "s"
#endif

struct sentry_path_s {
    sentry_pathchar_t *path;
};

struct sentry_filelock_s {
    void *_todo;
};

typedef struct sentry_path_s sentry_path_t;
typedef struct sentry_pathiter_s sentry_pathiter_t;
typedef struct sentry_filelock_s sentry_filelock_t;

sentry_path_t *sentry__path_current_exe(void);
sentry_path_t *sentry__path_dir(const sentry_path_t *path);
sentry_path_t *sentry__path_from_str(const char *s);
sentry_path_t *sentry__path_from_str_owned(char *s);
sentry_path_t *sentry__path_join_str(
    const sentry_path_t *base, const char *other);
sentry_path_t *sentry__path_clone(const sentry_path_t *path);
void sentry__path_free(sentry_path_t *path);

const sentry_pathchar_t *sentry__path_filename(const sentry_path_t *path);
bool sentry__path_filename_matches(
    const sentry_path_t *path, const char *filename);
bool sentry__path_ends_with(const sentry_path_t *path, const char *suffix);

bool sentry__path_is_dir(const sentry_path_t *path);
bool sentry__path_is_file(const sentry_path_t *path);
int sentry__path_remove(const sentry_path_t *path);
int sentry__path_remove_all(const sentry_path_t *path);
int sentry__path_create_dir_all(const sentry_path_t *path);
int sentry__path_touch(const sentry_path_t *path);
size_t sentry__path_get_size(const sentry_path_t *path);
char *sentry__path_read_to_buffer(const sentry_path_t *path, size_t *size_out);
int sentry__path_write_buffer(
    const sentry_path_t *path, const char *buf, size_t buf_len);
int sentry__path_append_buffer(
    const sentry_path_t *path, const char *buf, size_t buf_len);

sentry_pathiter_t *sentry__path_iter_directory(const sentry_path_t *path);
const sentry_path_t *sentry__pathiter_next(sentry_pathiter_t *piter);
void sentry__pathiter_free(sentry_pathiter_t *piter);

bool sentry__path_is_locked(const sentry_path_t *path);
sentry_filelock_t sentry__path_lock(const sentry_path_t *path);
void sentry__path_unlock(sentry_filelock_t lock);

/* windows specific API additions */
#ifdef SENTRY_PLATFORM_WINDOWS
sentry_path_t *sentry__path_from_wstr(const wchar_t *s);
sentry_path_t *sentry__path_join_wstr(
    const sentry_path_t *base, const wchar_t *other);
#endif

/* platform abstraction helper */
static inline sentry_path_t *
sentry__path_new(sentry_pathchar_t *s)
{
#ifdef SENTRY_PLATFORM_WINDOWS
    return sentry__path_from_wstr(s);
#else
    return sentry__path_from_str(s);
#endif
}

#endif
