#include "sentry_crash_reserve.h"
#include "sentry_logger.h"

#include <string.h>

#if defined(SENTRY_PLATFORM_WINDOWS)
#    include <windows.h>
#    define SENTRY_CRASH_RESERVE_SUPPORTED
#elif defined(SENTRY_PLATFORM_UNIX) && !defined(SENTRY_PLATFORM_PS)
#    include <sys/mman.h>
#    if !defined(MAP_ANONYMOUS) && defined(SENTRY_PLATFORM_DARWIN)
#        define MAP_ANONYMOUS MAP_ANON
#    endif
#    define SENTRY_CRASH_RESERVE_SUPPORTED
#endif

static void *volatile g_reserve = NULL;
static size_t g_reserve_size = 0;

#ifdef SENTRY_CRASH_RESERVE_SUPPORTED
static void *
take_reserve(void)
{
#    ifdef SENTRY_PLATFORM_WINDOWS
    return InterlockedExchangePointer((void *volatile *)&g_reserve, NULL);
#    else
    return __atomic_exchange_n(&g_reserve, NULL, __ATOMIC_SEQ_CST);
#    endif
}

static void *
map_memory(size_t size)
{
#    ifdef SENTRY_PLATFORM_WINDOWS
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#    else
    void *ptr = mmap(
        NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return ptr == MAP_FAILED ? NULL : ptr;
#    endif
}
#endif

void
sentry__crash_reserve_alloc(size_t size)
{
    sentry__crash_reserve_release();
    if (size == 0) {
        return;
    }

#ifdef SENTRY_CRASH_RESERVE_SUPPORTED
    void *ptr = map_memory(size);
    if (!ptr) {
        SENTRY_WARNF(
            "failed to reserve %zu bytes of crash handler memory", size);
        return;
    }
    // touch the pages so they are backed by memory
    memset(ptr, 1, size);

    g_reserve_size = size;
    g_reserve = ptr;
    SENTRY_DEBUGF("reserved %zu bytes of crash handler memory", size);
#else
    SENTRY_WARN("the crash memory reserve is not supported on this platform");
#endif
}

void
sentry__crash_reserve_release(void)
{
#ifdef SENTRY_CRASH_RESERVE_SUPPORTED
    void *ptr = take_reserve();
    if (!ptr) {
        return;
    }
#    ifdef SENTRY_PLATFORM_WINDOWS
    VirtualFree(ptr, 0, MEM_RELEASE);
#    else
    munmap(ptr, g_reserve_size);
#    endif
#endif
}

size_t
sentry__crash_reserve_size(void)
{
    return g_reserve ? g_reserve_size : 0;
}
