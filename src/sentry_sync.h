#ifndef SENTRY_SYNC_H_INCLUDED
#define SENTRY_SYNC_H_INCLUDED

#include <assert.h>
#include <sentry.h>
#include <stdio.h>

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

typedef HANDLE sentry_threadid_t;
typedef struct sentry__winmutex_s sentry_mutex_t;
typedef CONDITION_VARIABLE sentry_cond_t;
#    define SENTRY__MUTEX_INIT NULL
#    define sentry__mutex_lock(Lock) sentry__winmutex_lock(&(Lock))
#    define sentry__mutex_unlock(Lock)                                         \
        LeaveCriticalSection(&(Lock)->critical_section)
#    define sentry__cond_wait_timeout(CondVar, Lock, Timeout)                  \
        SleepConditionVariableCS(CondVar, &Lock->critical_section, Timeout)
#    define sentry__cond_wait(CondVar, Lock)                                   \
        sentry__cond_wait_timeout(CondVar, Lock, INFINITE)
#    define sentry__cond_wake pthread_cond_wake WakeConditionVariable
#    define sentry__thread_spawn(ThreadId, Func, Data)                         \
        CreateThread(NULL, 0, Func, Data, 0, ThreadId)
#    define sentry__thread_join(ThreadId)                                      \
        WaitForSingleObject(ThreadId, INFINITE)
#else
#    include <pthread.h>
#    include <sys/time.h>
typedef pthread_t sentry_threadid_t;
typedef pthread_mutex_t sentry_mutex_t;
typedef pthread_cond_t sentry_cond_t;
#    define SENTRY__MUTEX_INIT PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#    define sentry__mutex_lock(Mutex)                                          \
        do {                                                                   \
            int _rv = pthread_mutex_lock(Mutex);                               \
            assert(_rv == 0);                                                  \
        } while (0)
#    define sentry__mutex_unlock pthread_mutex_unlock
#    define SENTRY__COND_INIT PTHREAD_COND_INITIALIZER
#    define sentry__cond_wait pthread_cond_wait
#    define sentry__cond_wake pthread_cond_signal
#    define sentry__thread_spawn(ThreadId, Func, Data)                         \
        (pthread_create(ThreadId, NULL, (void *(*)(void *))Func, Data) == 0)
#    define sentry__thread_join(ThreadId) pthread_join(ThreadId, NULL)

static inline int
sentry__cond_wait_timeout(
    sentry_cond_t *cv, sentry_mutex_t *mutex, u_int64_t msecs)
{
    struct timeval now;
    struct timespec lock_time;
    gettimeofday(&now, NULL);
    lock_time.tv_sec = now.tv_sec + msecs / 1000ULL;
    lock_time.tv_nsec = (now.tv_usec + 1000ULL * (msecs % 1000)) * 1000ULL;
    return pthread_cond_timedwait(cv, mutex, &lock_time);
}
#endif
#define sentry__mutex_init(Mutex)                                              \
    do {                                                                       \
        sentry_mutex_t tmp = SENTRY__MUTEX_INIT;                               \
        *(Mutex) = tmp;                                                        \
    } while (0)
#define sentry__cond_init(CondVar)                                             \
    do {                                                                       \
        sentry_cond_t tmp = SENTRY__COND_INIT;                                 \
        *(CondVar) = tmp;                                                      \
    } while (0)

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

struct sentry_bgworker_s;
typedef struct sentry_bgworker_s sentry_bgworker_t;

typedef void (*sentry_task_function_t)(void *data);

sentry_bgworker_t *sentry__bgworker_new(void);
void sentry__bgworker_free(sentry_bgworker_t *bgw);
void sentry__bgworker_start(sentry_bgworker_t *bgw);
int sentry__bgworker_shutdown(sentry_bgworker_t *bgw);
int sentry__bgworker_submit(sentry_bgworker_t *bgw,
    sentry_task_function_t exec_func, sentry_task_function_t cleanup_func,
    void *data);

#endif