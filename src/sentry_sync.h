#ifndef SENTRY_SYNC_H_INCLUDED
#define SENTRY_SYNC_H_INCLUDED

#include <sentry.h>

/* define a recursive mutex for all platforms */
#if SENTRY_PLATFORM == SENTRY_PLATFORM_WINDOWS
#    include <synchapi.h>
#    include <winnt.h>
struct sentry__winmutex_s {
    INIT_ONCE init_once;
    CRITICAL_SECTION critical_section;
};

static inline bool CALLBACK
sentry__winmutex_init(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
    InitializeCriticalSection(lpContext);
    return true;
}

static inline void
sentry__winmutex_lock(volatile struct sentry__winmutex_s *mutex)
{
    PVOID lpContext = &mutex->critical_section;
    InitOnceExecuteOnce(
        &mutex->init_once, sentry__winmutex_init, NULL, lpContext);
    if (!InterlockedExchangeAdd(mutex->initialized, 0)) {
        CRITICAL_SECTION s;
        InitializeCriticalSection(&s);
    }
    EnterCriticalSection(&mutex->section);
}

typedef pthread_mutex_t struct sentry__winmutex_s;
#    define SENTRY__MUTEX_INIT NULL
#    define sentry__mutex_lock(Lock) sentry__winmutex_lock(&(Lock))
#    define sentry__mutex_unlock(Lock)                                         \
        LeaveCriticalSection(&(Lock)->critical_section)
#else
#    include <pthread.h>
typedef pthread_mutex_t sentry_mutex_t;
#    define SENTRY__MUTEX_INIT PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#    define sentry__mutex_lock pthread_mutex_lock
#    define sentry__mutex_unlock pthread_mutex_unlock
#endif

static inline int
sentry__atomic_fetch_and_add(volatile int *val, int diff)
{
#if SENTRY_PLATFORM == SENTRY_PLATFORM_WINDOWS
    return ::InterlockedExchangeAdd(ptr, value);
#else
    return __sync_fetch_and_add(val, diff);
#endif
}

static inline int
sentry__atomic_fetch(volatile int *val)
{
    return sentry__atomic_fetch_and_add(val, 0);
}

#endif