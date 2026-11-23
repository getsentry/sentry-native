#include "sentry.h"

#include <stdlib.h>

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#    define SANITIZER_BUILD
#elif defined(__has_feature)
#    if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#        define SANITIZER_BUILD
#    endif
#endif

#if defined(SENTRY_PLATFORM_WINDOWS)
#    include <windows.h>
#    define HAS_MALLOC_LOCK
#elif defined(SENTRY_PLATFORM_DARWIN)
#    include <malloc/malloc.h>
#    include <pthread.h>
#    define HAS_MALLOC_LOCK
#elif defined(__GLIBC__) && !defined(SANITIZER_BUILD)
#    include <pthread.h>
#    include <unistd.h>
#    define HAS_MALLOC_LOCK
#    define MALLOC_INTERPOSER
#endif

#ifdef MALLOC_INTERPOSER
// glibc has no public allocator lock, so replace malloc and block every caller
// once locked
extern void *__libc_malloc(size_t size);
extern void *__libc_calloc(size_t nmemb, size_t size);
extern void *__libc_realloc(void *ptr, size_t size);
extern void __libc_free(void *ptr);

static int g_locked;

static void
wait_unlocked(void)
{
    while (__atomic_load_n(&g_locked, __ATOMIC_ACQUIRE)) {
        pause();
    }
}

void *
malloc(size_t size)
{
    wait_unlocked();
    return __libc_malloc(size);
}

void *
calloc(size_t nmemb, size_t size)
{
    wait_unlocked();
    return __libc_calloc(nmemb, size);
}

void *
realloc(void *ptr, size_t size)
{
    wait_unlocked();
    return __libc_realloc(ptr, size);
}

void
free(void *ptr)
{
    wait_unlocked();
    __libc_free(ptr);
}
#endif

typedef void (*crash_func_t)(void);

#ifdef HAS_MALLOC_LOCK
static void
lock_malloc(void)
{
#    if defined(SENTRY_PLATFORM_WINDOWS)
    HeapLock(GetProcessHeap());
#    elif defined(SENTRY_PLATFORM_DARWIN)
    malloc_zone_t *zone = malloc_default_zone();
    zone->introspect->force_lock(zone);
#    else
    __atomic_store_n(&g_locked, 1, __ATOMIC_RELEASE);
#    endif
}

#    ifdef SENTRY_PLATFORM_WINDOWS
static DWORD WINAPI
crash_thread_func(LPVOID lpParam)
{
    lock_malloc();
    (*(crash_func_t *)lpParam)();
    return 0;
}
#    else
static void *
crash_thread_func(void *arg)
{
    lock_malloc();
    (*(crash_func_t *)arg)();
    return NULL;
}
#    endif
#endif

void
crash_malloc_locked(crash_func_t crash)
{
    // crash on a fresh thread that has not touched sentry's thread-locals
#if defined(SENTRY_PLATFORM_WINDOWS)
    HANDLE thread = CreateThread(NULL, 0, crash_thread_func, &crash, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#elif defined(HAS_MALLOC_LOCK)
    pthread_t thread;
    pthread_create(&thread, NULL, crash_thread_func, &crash);
    pthread_join(thread, NULL);
#else
    (void)crash;
#endif
}
