#include "sentry_thread_stackwalk.h"

#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_WINDOWS)

#    include "sentry.h"
#    include "sentry_logger.h"

#    include <string.h>
#    include <windows.h>

#    if !defined(SENTRY_PLATFORM_XBOX)
#        include <tlhelp32.h>
#    endif

#    if defined(_MSC_VER) && defined(_M_X64)
#        define SENTRY_THREAD_STACKWALK_RTL_UNWIND 1
#    else
#        define SENTRY_THREAD_STACKWALK_RTL_UNWIND 0
#    endif

#    if SENTRY_THREAD_STACKWALK_RTL_UNWIND
// Not in the SDK headers: the low bit of `UnwindData` marks an entry that
// points to another `RUNTIME_FUNCTION` instead of to unwind info.
#        define RUNTIME_FUNCTION_INDIRECTION 0x1

// Finds the unwind data for `pc` in the exception directory of its image.
// `RtlLookupFunctionEntry` cannot be used: it takes the loader's function
// table lock, which the suspended thread can hold while it loads or unloads a
// module. Sets `in_image` when `pc` is in a loaded image.
static PRUNTIME_FUNCTION
find_function_entry(DWORD64 pc, DWORD64 *image_base, bool *in_image)
{
    *in_image = false;
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery((LPCVOID)pc, &mbi, sizeof(mbi))
        || mbi.Type != MEM_IMAGE) {
        return NULL;
    }
    const BYTE *base = (const BYTE *)mbi.AllocationBase;
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return NULL;
    }
    const IMAGE_NT_HEADERS64 *nt
        = (const IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE
        || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return NULL;
    }
    *in_image = true;
    *image_base = (DWORD64)base;

    const IMAGE_DATA_DIRECTORY *dir
        = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    if (dir->VirtualAddress == 0) {
        return NULL;
    }
    PRUNTIME_FUNCTION table = (PRUNTIME_FUNCTION)(base + dir->VirtualAddress);
    const DWORD rva = (DWORD)(pc - *image_base);
    size_t lo = 0;
    size_t hi = dir->Size / sizeof(RUNTIME_FUNCTION);
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        if (rva < table[mid].BeginAddress) {
            hi = mid;
        } else if (rva >= table[mid].EndAddress) {
            lo = mid + 1;
        } else if (table[mid].UnwindData & RUNTIME_FUNCTION_INDIRECTION) {
            return (PRUNTIME_FUNCTION)(base + table[mid].UnwindData
                - RUNTIME_FUNCTION_INDIRECTION);
        } else {
            return &table[mid];
        }
    }
    return NULL;
}

// Unwinds without taking any user-mode lock or allocating, because the walked
// thread is suspended and can hold any of them. A corrupt stack can make the
// unwind read invalid memory, so the walk runs inside SEH.
static size_t
unwind_rtl(const CONTEXT *thread_ctx, void **ips, size_t max)
{
    CONTEXT ctx = *thread_ctx;
    size_t n = 0;
    __try {
        while (n < max && ctx.Rip != 0) {
            ips[n++] = (void *)ctx.Rip;

            DWORD64 image_base = 0;
            bool in_image = false;
            PRUNTIME_FUNCTION fn
                = find_function_entry(ctx.Rip, &image_base, &in_image);
            if (fn) {
                PVOID handler_data = NULL;
                DWORD64 establisher_frame = 0;
                RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, ctx.Rip, fn,
                    &ctx, &handler_data, &establisher_frame, NULL);
            } else if (in_image) {
                // A leaf function has no unwind data: its return address is
                // at the top of the stack.
                ctx.Rip = *(DWORD64 *)ctx.Rsp;
                ctx.Rsp += 8;
            } else {
                // Code outside an image (JIT) has no unwind data we can read.
                break;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Keep the frames that were walked before the fault.
    }
    return n;
}
#    else
static size_t
unwind_dbghelp(CONTEXT *ctx, void **ips, size_t max)
{
    EXCEPTION_RECORD er;
    memset(&er, 0, sizeof(er));
    EXCEPTION_POINTERS ep;
    ep.ExceptionRecord = &er;
    ep.ContextRecord = ctx;
    sentry_ucontext_t s;
    memset(&s, 0, sizeof(s));
    s.exception_ptrs = ep;
    // NOTE: the shared dbghelp unwinder drives StackWalk64 with
    // GetCurrentThread() (the watchdog's pseudo-handle), not the suspended
    // target thread. On x64 the walk is CONTEXT-driven and reads the target's
    // stack from the shared process address space, so the captured IPs are
    // correct. On x86 (frame-pointer mode) and ARM64, StackWalk64 may use the
    // thread handle for register updates, so cross-thread unwinding there
    // needs the target handle threaded through the unwinder. Tracked as a
    // follow-up; x64 is the validated path for this experimental feature.
    return sentry_unwind_stack_from_ucontext(&s, ips, max);
}
#    endif

size_t
sentry__thread_stackwalk(uint64_t target_tid, void **ips, size_t max)
{
    HANDLE h = OpenThread(
        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
        FALSE, (DWORD)target_tid);
    if (!h) {
        return 0;
    }

    size_t n = 0;
    DWORD context_error = 0;
    if (SuspendThread(h) != (DWORD)-1) {
        CONTEXT ctx;
        memset(&ctx, 0, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_FULL;
        if (GetThreadContext(h, &ctx)) { // forces the suspend to complete
#    if SENTRY_THREAD_STACKWALK_RTL_UNWIND
            n = unwind_rtl(&ctx, ips, max);
#    else
            n = unwind_dbghelp(&ctx, ips, max);
#    endif
        } else {
            context_error = GetLastError();
        }
        ResumeThread(h); // ALWAYS resume
    }
    CloseHandle(h);
    // Log only after the resume: the logger can take a lock that the
    // suspended thread holds.
    if (context_error) {
        SENTRY_DEBUGF("app-hang: GetThreadContext failed: %lu", context_error);
    }
    return n;
}

#    if defined(SENTRY_PLATFORM_XBOX)
// ToolHelp32 is not part of the gaming partition.
size_t
sentry__thread_list(sentry_thread_info_t *out, size_t max)
{
    (void)out;
    (void)max;
    return 0;
}
#    else
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
        WideCharToMultiByte(
            CP_UTF8, 0, description, -1, name, (int)name_size, NULL, NULL);
        name[name_size - 1] = '\0';
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
