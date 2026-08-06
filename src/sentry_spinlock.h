#ifndef SENTRY_SPINLOCK_H_INCLUDED
#define SENTRY_SPINLOCK_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_cpu_relax.h"

typedef volatile long sentry_spinlock_t;
typedef bool (*sentry_spinlock_wait_func_t)(int attempt, void *data);

#define SENTRY__SPINLOCK_INIT 0

static inline bool
sentry__spinlock_try_lock(sentry_spinlock_t *spinlock)
{
#ifdef SENTRY_PLATFORM_WINDOWS
#    if SIZEOF_LONG == 8
    return InterlockedCompareExchange64((volatile LONG64 *)spinlock, 1, 0) == 0;
#    else
    return InterlockedCompareExchange((volatile LONG *)spinlock, 1, 0) == 0;
#    endif
#else
    long unlocked = 0;
    return __atomic_compare_exchange_n(
        spinlock, &unlocked, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
#endif
}

static inline void
sentry__spinlock_lock(sentry_spinlock_t *spinlock)
{
    while (!sentry__spinlock_try_lock(spinlock)) {
        sentry__cpu_relax();
    }
}

static inline bool
sentry__spinlock_wait(sentry_spinlock_t *spinlock,
    sentry_spinlock_wait_func_t wait_func, void *data)
{
    int attempts = 0;
    while (!sentry__spinlock_try_lock(spinlock)) {
        if (!wait_func || !wait_func(++attempts, data)) {
            return false;
        }
    }
    return true;
}

static inline void
sentry__spinlock_unlock(sentry_spinlock_t *spinlock)
{
#ifdef SENTRY_PLATFORM_WINDOWS
#    if SIZEOF_LONG == 8
    InterlockedExchange64((volatile LONG64 *)spinlock, 0);
#    else
    InterlockedExchange((volatile LONG *)spinlock, 0);
#    endif
#else
    __atomic_store_n(spinlock, 0, __ATOMIC_RELEASE);
#endif
}

#endif
