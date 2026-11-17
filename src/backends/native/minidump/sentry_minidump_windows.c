#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_WINDOWS)

#    include <dbghelp.h>
#    include <stdio.h>
#    include <string.h>
#    include <windows.h>

#    include "sentry.h"
#    include "sentry_logger.h"
#    include "sentry_minidump_writer.h"
#    include "sentry_string.h"

/**
 * On Xbox, MiniDumpWriteDump is provided by xgameplatform.lib
 * (linked by the Xbox toolchain), not dbghelp.lib.
 */
#    if defined(_MSC_VER) && !defined(SENTRY_PLATFORM_XBOX)
#        pragma comment(lib, "dbghelp.lib")
#    endif

#    define IMAGE_DATA_PAGE_SIZE 4096
#    define IMAGE_DATA_MAX_PAGES 256

/**
 * Pages of loaded images (.rdata, .data) that thread stacks point to.
 * MiniDumpWithIndirectlyReferencedMemory skips memory inside images, so these
 * are added through the MemoryCallback of MiniDumpWriteDump instead.
 */
typedef struct {
    HANDLE process;
    MEMORY_BASIC_INFORMATION region;
    uint64_t pages[IMAGE_DATA_MAX_PAGES];
    size_t count;
    size_t next;
} image_data_t;

static bool
is_image_data(const MEMORY_BASIC_INFORMATION *region)
{
    if (region->State != MEM_COMMIT || region->Type != MEM_IMAGE
        || (region->Protect & PAGE_GUARD) != 0) {
        return false;
    }
    DWORD protect = region->Protect & 0xFF;
    return protect == PAGE_READONLY || protect == PAGE_READWRITE
        || protect == PAGE_WRITECOPY;
}

static void
consider_image_data(image_data_t *data, uintptr_t addr)
{
    if (data->count >= IMAGE_DATA_MAX_PAGES || addr < 0x10000) {
        return;
    }
    uint64_t page = addr & ~(uint64_t)(IMAGE_DATA_PAGE_SIZE - 1);
    for (size_t i = 0; i < data->count; i++) {
        if (data->pages[i] == page) {
            return;
        }
    }
    // Stack words often point into the same region, so reuse the last query.
    uintptr_t base = (uintptr_t)data->region.BaseAddress;
    if (addr < base || addr - base >= data->region.RegionSize) {
        if (!VirtualQueryEx(data->process, (LPCVOID)addr, &data->region,
                sizeof(data->region))) {
            memset(&data->region, 0, sizeof(data->region));
            return;
        }
    }
    if (is_image_data(&data->region)) {
        data->pages[data->count++] = page;
    }
}

static void
scan_stack(image_data_t *data, ULONG64 start, ULONG64 end)
{
    ULONG64 lo = start < end ? start : end;
    ULONG64 hi = start < end ? end : start;
    SIZE_T size = (SIZE_T)(hi - lo);
    void *buf = size ? sentry_malloc(size) : NULL;
    if (!buf) {
        return;
    }
    SIZE_T nread = 0;
    ReadProcessMemory(data->process, (LPCVOID)(uintptr_t)lo, buf, size, &nread);
    const uintptr_t *words = (const uintptr_t *)buf;
    for (size_t i = 0; i < nread / sizeof(uintptr_t); i++) {
        consider_image_data(data, words[i]);
    }
    sentry_free(buf);
}

static BOOL CALLBACK
image_data_callback(PVOID param, PMINIDUMP_CALLBACK_INPUT input,
    PMINIDUMP_CALLBACK_OUTPUT output)
{
    image_data_t *data = (image_data_t *)param;
    switch (input->CallbackType) {
    case IncludeModuleCallback:
    case ModuleCallback:
    case IncludeThreadCallback:
        return TRUE;
    case ThreadCallback:
    case ThreadExCallback:
        // MINIDUMP_THREAD_EX_CALLBACK starts with the same fields.
        scan_stack(data, input->Thread.StackBase, input->Thread.StackEnd);
        return TRUE;
    case MemoryCallback:
        if (data->next >= data->count) {
            return FALSE;
        }
        output->MemoryBase = data->pages[data->next++];
        output->MemorySize = IMAGE_DATA_PAGE_SIZE;
        return TRUE;
    case CancelCallback:
        output->CheckCancel = FALSE;
        output->Cancel = FALSE;
        return TRUE;
    default:
        return FALSE;
    }
}

/**
 * Windows minidump writer
 * Windows provides MiniDumpWriteDump API which does all the heavy lifting!
 */
int
sentry__write_minidump(
    const sentry_crash_context_t *ctx, const char *output_path)
{
    SENTRY_DEBUGF("writing minidump to %s", output_path);

    // Open output file - use wide character API for proper UTF-8 path support
    wchar_t *woutput_path = sentry__string_to_wstr(output_path);
    if (!woutput_path) {
        SENTRY_WARN("failed to convert minidump path to wide string");
        return -1;
    }

    HANDLE file_handle = CreateFileW(woutput_path, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file_handle == INVALID_HANDLE_VALUE) {
        SENTRY_WARNF("failed to create minidump file: %lu", GetLastError());
        sentry_free(woutput_path);
        return -1;
    }
    sentry_free(woutput_path);

    image_data_t image_data = { 0 };
    MINIDUMP_CALLBACK_INFORMATION image_data_info
        = { image_data_callback, &image_data };
    PMINIDUMP_CALLBACK_INFORMATION callback = NULL;

    MINIDUMP_TYPE dump_type;
    if (ctx->minidump_flags >= 0) {
        dump_type = (MINIDUMP_TYPE)ctx->minidump_flags;
    } else {
        switch (ctx->minidump_mode) {
        case SENTRY_MINIDUMP_MODE_STACK_ONLY:
            dump_type = MiniDumpNormal;
            break;

        case SENTRY_MINIDUMP_MODE_SMART:
            // Only indirectly-referenced memory: dbghelp walks the stack and
            // registers and captures a small window around each pointer that
            // lands in mapped memory. We deliberately omit MiniDumpWithDataSegs
            // here: it writes the entire writable data segment of *every*
            // loaded module (all DLL .data/.bss), which is uncapped and
            // dominates dump size — for large module graphs it produces dumps
            // 10-30x larger than intended. Global/static data is rarely needed
            // for crash triage; callers that want it can opt in via
            // sentry_options_set_minidump_flags() or MODE_FULL.
            dump_type = MiniDumpWithIndirectlyReferencedMemory;
            callback = &image_data_info;
            break;

        case SENTRY_MINIDUMP_MODE_FULL:
            dump_type = MiniDumpWithFullMemory | MiniDumpWithHandleData
                | MiniDumpWithThreadInfo;
            break;

        default:
            dump_type = MiniDumpNormal;
            break;
        }
    }

    // Open crashed process with permissions needed for MiniDumpWriteDump.
    // MiniDumpWithHandleData requires PROCESS_DUP_HANDLE.
    DWORD access_flags = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    if ((dump_type & MiniDumpWithHandleData) != 0) {
        access_flags |= PROCESS_DUP_HANDLE;
    }
    HANDLE process_handle = OpenProcess(access_flags, FALSE, ctx->crashed_pid);

    if (process_handle == NULL) {
        SENTRY_WARNF("failed to open process %lu: %lu", ctx->crashed_pid,
            GetLastError());
        CloseHandle(file_handle);
        wchar_t *wdelete_path = sentry__string_to_wstr(output_path);
        if (wdelete_path) {
            DeleteFileW(wdelete_path);
            sentry_free(wdelete_path);
        }
        return -1;
    }
    image_data.process = process_handle;

    // Prepare exception information using original pointers from crashed
    // process
    EXCEPTION_POINTERS local_exception_pointers = { 0 };
    EXCEPTION_RECORD local_exception_record = { 0 };
    MINIDUMP_EXCEPTION_INFORMATION exception_info = { 0 };
    exception_info.ThreadId = ctx->crashed_tid;
    if (ctx->platform.exception_pointers) {
        // Use original exception pointers from crashed process's address space
        exception_info.ExceptionPointers = ctx->platform.exception_pointers;
        // ClientPointers=TRUE tells Windows these pointers are in the target
        // process
        exception_info.ClientPointers = TRUE;
    } else {
        // WER copies exception data into shared memory, so ClientPointers must
        // be false when using the copied record/context.
        local_exception_record = ctx->platform.exception_record;
        local_exception_record.ExceptionRecord = NULL;
        local_exception_pointers.ExceptionRecord = &local_exception_record;
        local_exception_pointers.ContextRecord
            = (PCONTEXT)&ctx->platform.context;
        exception_info.ExceptionPointers = &local_exception_pointers;
        exception_info.ClientPointers = FALSE;
    }

    // Write minidump using Windows API
    BOOL success = MiniDumpWriteDump(process_handle, ctx->crashed_pid,
        file_handle, dump_type, &exception_info, NULL, callback);

    DWORD error = GetLastError();

    CloseHandle(process_handle);
    CloseHandle(file_handle);

    if (!success) {
        SENTRY_WARNF("MiniDumpWriteDump failed: %lu", error);
        wchar_t *wdelete_path2 = sentry__string_to_wstr(output_path);
        if (wdelete_path2) {
            DeleteFileW(wdelete_path2);
            sentry_free(wdelete_path2);
        }
        return -1;
    }

    SENTRY_DEBUG("successfully wrote minidump");
    return 0;
}

#endif // SENTRY_PLATFORM_WINDOWS
