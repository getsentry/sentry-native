#include "sentry_thread_stackwalk.h"

#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_WINDOWS)

#    include "sentry.h"
#    include "sentry_logger.h"

#    include <string.h>
#    include <windows.h>

#    if SENTRY_HAS_THREAD_LIST
#        include <tlhelp32.h>
#    endif

size_t
sentry__thread_stackwalk(
    uint64_t target_tid, void **ips, size_t max, uint64_t timeout_ms)
{
    (void)timeout_ms;
    HANDLE h = OpenThread(
        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
        FALSE, (DWORD)target_tid);
    if (!h) {
        return 0;
    }

    size_t n = 0;
    if (SuspendThread(h) != (DWORD)-1) {
        CONTEXT ctx;
        memset(&ctx, 0, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_FULL;
        if (GetThreadContext(h, &ctx)) { // forces the suspend to complete
            EXCEPTION_RECORD er;
            memset(&er, 0, sizeof(er));
            EXCEPTION_POINTERS ep;
            ep.ExceptionRecord = &er;
            ep.ContextRecord = &ctx;
            sentry_ucontext_t s;
            memset(&s, 0, sizeof(s));
            s.exception_ptrs = ep;
            // NOTE: the shared dbghelp unwinder drives StackWalk64 with
            // GetCurrentThread() (the watchdog's pseudo-handle), not the
            // suspended target thread. On x64 and ARM64 the walk is
            // CONTEXT-driven and reads the target's stack from the shared
            // process address space, so the captured IPs are correct. On x86
            // (frame-pointer mode), StackWalk64 may use the thread handle for
            // register updates, so cross-thread unwinding there needs the
            // target handle threaded through the unwinder. Tracked as a
            // follow-up; x64 and ARM64 are the validated paths for this
            // experimental feature.
            n = sentry_unwind_stack_from_ucontext(&s, ips, max);
        } else {
            SENTRY_DEBUGF(
                "app-hang: GetThreadContext failed: %lu", GetLastError());
        }
        ResumeThread(h); // ALWAYS resume
    }
    CloseHandle(h);
    return n;
}

#    if SENTRY_HAS_THREAD_LIST
/**
 * Retrieve the thread name via GetThreadDescription (Windows 10 1607+).
 */
static void
get_thread_name(DWORD tid, char *name, size_t name_size)
{
    typedef HRESULT(WINAPI * GetThreadDescription_t)(HANDLE, PWSTR *);
    static GetThreadDescription_t get_thread_description
        = (GetThreadDescription_t)-1;
    if (get_thread_description == (GetThreadDescription_t)-1) {
        get_thread_description = (GetThreadDescription_t)GetProcAddress(
            GetModuleHandleW(L"kernel32.dll"), "GetThreadDescription");
    }

    name[0] = '\0';
    if (!get_thread_description) {
        return;
    }
    HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
    if (!thread) {
        return;
    }
    PWSTR description = NULL;
    HRESULT hr = get_thread_description(thread, &description);
    if (SUCCEEDED(hr) && description && description[0] != L'\0') {
        if (!WideCharToMultiByte(CP_UTF8, 0, description, -1, name,
                (int)name_size, NULL, NULL)) {
            name[0] = '\0';
        }
    }
    LocalFree(description);
    CloseHandle(thread);
}

size_t
sentry__thread_list(sentry_thread_info_t *out, size_t max)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    const DWORD pid = GetCurrentProcessId();
    size_t count = 0;
    THREADENTRY32 entry;
    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry)) {
        do {
            if (entry.th32OwnerProcessID != pid) {
                continue;
            }
            out[count].tid = entry.th32ThreadID;
            get_thread_name(
                entry.th32ThreadID, out[count].name, sizeof(out[count].name));
            count++;
        } while (count < max && Thread32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return count;
}
#    endif

#endif
