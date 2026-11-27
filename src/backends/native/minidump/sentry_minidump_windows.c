#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_WINDOWS)

#    include <dbghelp.h>
#    include <stdio.h>
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

// PROBE: temporary diagnostics for ERROR_PARTIAL_COPY on CI
static BOOL CALLBACK
probe_callback(PVOID param, const PMINIDUMP_CALLBACK_INPUT input,
    PMINIDUMP_CALLBACK_OUTPUT output)
{
    if (input->CallbackType == ReadMemoryFailureCallback) {
        HANDLE process = (HANDLE)param;
        ULONG64 offset = input->ReadMemoryFailure.Offset;
        MEMORY_BASIC_INFORMATION mbi = { 0 };
        VirtualQueryEx(process, (LPCVOID)(ULONG_PTR)offset, &mbi, sizeof(mbi));
        SENTRY_WARNF("PROBE read failure: offset=0x%llx bytes=%lu "
                     "status=0x%lx base=%p alloc_base=%p alloc_protect=0x%lx "
                     "size=0x%llx state=0x%lx protect=0x%lx type=0x%lx",
            (unsigned long long)offset, input->ReadMemoryFailure.Bytes,
            (unsigned long)input->ReadMemoryFailure.FailureStatus,
            mbi.BaseAddress, mbi.AllocationBase, mbi.AllocationProtect,
            (unsigned long long)mbi.RegionSize, mbi.State, mbi.Protect,
            mbi.Type);
        output->Status = S_OK;
    }
    return TRUE;
}

static void
probe_minidump(const sentry_crash_context_t *ctx, const char *output_path,
    const char *label, MINIDUMP_TYPE dump_type, int exception_mode,
    BOOL callback)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s.probe", output_path);
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ctx->crashed_pid);
    if (file == INVALID_HANDLE_VALUE || !process) {
        SENTRY_WARNF("PROBE %s: setup failed %lu", label, GetLastError());
        return;
    }

    EXCEPTION_POINTERS pointers = { 0 };
    EXCEPTION_RECORD record = ctx->platform.exception_record;
    record.ExceptionRecord = NULL;
    pointers.ExceptionRecord = &record;
    pointers.ContextRecord = (PCONTEXT)&ctx->platform.context;
    MINIDUMP_EXCEPTION_INFORMATION info = { 0 };
    info.ThreadId = ctx->crashed_tid;
    if (exception_mode == 1) {
        info.ExceptionPointers = ctx->platform.exception_pointers;
        info.ClientPointers = TRUE;
    } else {
        info.ExceptionPointers = &pointers;
        info.ClientPointers = FALSE;
    }
    MINIDUMP_CALLBACK_INFORMATION cb = { probe_callback, process };

    BOOL ok = MiniDumpWriteDump(process, ctx->crashed_pid, file, dump_type,
        exception_mode ? &info : NULL, NULL, callback ? &cb : NULL);
    DWORD error = ok ? 0 : GetLastError();
    LARGE_INTEGER size = { 0 };
    GetFileSizeEx(file, &size);
    SENTRY_WARNF("PROBE %s: ok=%d error=%lu size=%lld", label, ok, error,
        (long long)size.QuadPart);
    CloseHandle(process);
    CloseHandle(file);
    DeleteFileA(path);
}

static void
probe_context(const sentry_crash_context_t *ctx)
{
    DWORD context_len = 0;
    InitializeContext(
        NULL, ctx->platform.context.ContextFlags, NULL, &context_len);
    SENTRY_WARNF("PROBE local: context_flags=0x%lx sizeof(CONTEXT)=%zu "
                 "enabled_xstate=0x%llx context_len=0x%lx",
        ctx->platform.context.ContextFlags, sizeof(CONTEXT),
        (unsigned long long)GetEnabledXStateFeatures(), context_len);
    HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ctx->crashed_pid);
    if (!process) {
        return;
    }
    EXCEPTION_POINTERS pointers = { 0 };
    SIZE_T read = 0;
    BOOL ok = ReadProcessMemory(process, ctx->platform.exception_pointers,
        &pointers, sizeof(pointers), &read);
    SENTRY_WARNF("PROBE pointers: ok=%d record=%p context=%p", ok,
        (void *)pointers.ExceptionRecord, (void *)pointers.ContextRecord);
    if (ok && pointers.ContextRecord) {
        MEMORY_BASIC_INFORMATION mbi = { 0 };
        VirtualQueryEx(process, pointers.ContextRecord, &mbi, sizeof(mbi));
        ULONG_PTR end = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize;
        SENTRY_WARNF("PROBE context region: base=%p size=0x%llx "
                     "headroom=0x%llx state=0x%lx protect=0x%lx",
            mbi.BaseAddress, (unsigned long long)mbi.RegionSize,
            (unsigned long long)(end - (ULONG_PTR)pointers.ContextRecord),
            mbi.State, mbi.Protect);
        DWORD flags = 0;
        ok = ReadProcessMemory(process,
            (char *)pointers.ContextRecord + offsetof(CONTEXT, ContextFlags),
            &flags, sizeof(flags), &read);
        SENTRY_WARNF(
            "PROBE remote context_flags: ok=%d flags=0x%lx", ok, flags);
        struct {
            LONG offset;
            DWORD length;
        } chunks[3] = { 0 };
        ok = ReadProcessMemory(process,
            (char *)pointers.ContextRecord + sizeof(CONTEXT), chunks,
            sizeof(chunks), &read);
        SENTRY_WARNF("PROBE context_ex: ok=%d all=(%ld,%lu) legacy=(%ld,%lu) "
                     "xstate=(%ld,%lu)",
            ok, chunks[0].offset, chunks[0].length, chunks[1].offset,
            chunks[1].length, chunks[2].offset, chunks[2].length);
        if (ok) {
            char *ex = (char *)pointers.ContextRecord + sizeof(CONTEXT);
            ULONG_PTR all_end
                = (ULONG_PTR)ex + chunks[0].offset + chunks[0].length;
            ULONG_PTR xstate_end
                = (ULONG_PTR)ex + chunks[2].offset + chunks[2].length;
            SENTRY_WARNF(
                "PROBE context_ex ends: all_end=0x%llx xstate_end=0x%llx "
                "region_end=0x%llx",
                (unsigned long long)all_end, (unsigned long long)xstate_end,
                (unsigned long long)end);
        }
    }
    if (ok && pointers.ExceptionRecord) {
        EXCEPTION_RECORD record = { 0 };
        ok = ReadProcessMemory(
            process, pointers.ExceptionRecord, &record, sizeof(record), &read);
        SENTRY_WARNF("PROBE remote record: ok=%d read=%zu chained=%p", ok, read,
            (void *)record.ExceptionRecord);
    }
    CloseHandle(process);
}

static void
probe_failure(const sentry_crash_context_t *ctx, const char *output_path,
    MINIDUMP_TYPE dump_type)
{
    const EXCEPTION_RECORD *rec = &ctx->platform.exception_record;
    SENTRY_WARNF("PROBE exception: code=0x%lx flags=0x%lx address=%p "
                 "params=%lu p0=0x%llx p1=0x%llx pointers=%p tid=%lu "
                 "dump_type=0x%x",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        rec->NumberParameters, (unsigned long long)rec->ExceptionInformation[0],
        (unsigned long long)rec->ExceptionInformation[1],
        (void *)ctx->platform.exception_pointers, ctx->crashed_tid,
        (unsigned)dump_type);
    probe_minidump(ctx, output_path, "same", dump_type, 1, FALSE);
    probe_minidump(ctx, output_path, "local-pointers", dump_type, 2, FALSE);
    probe_minidump(ctx, output_path, "no-exception", dump_type, 0, FALSE);
    probe_minidump(ctx, output_path, "normal", MiniDumpNormal, 1, FALSE);
    probe_minidump(ctx, output_path, "callback", dump_type, 1, TRUE);
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

    if (exception_info.ClientPointers) {
        probe_context(ctx);
    }

    // Write minidump using Windows API
    BOOL success = MiniDumpWriteDump(process_handle, ctx->crashed_pid,
        file_handle, dump_type, &exception_info, NULL, NULL);

    DWORD error = GetLastError();

    if (!success && exception_info.ClientPointers) {
        // dbghelp may fail to read the remote extended context near the top
        // of the crashed thread's stack. Retry with the copied record/context.
        SENTRY_WARNF("MiniDumpWriteDump failed with client pointers: %lu, "
                     "retrying with copied context",
            error);
        local_exception_record = ctx->platform.exception_record;
        local_exception_record.ExceptionRecord = NULL;
        local_exception_pointers.ExceptionRecord = &local_exception_record;
        local_exception_pointers.ContextRecord
            = (PCONTEXT)&ctx->platform.context;
        exception_info.ExceptionPointers = &local_exception_pointers;
        exception_info.ClientPointers = FALSE;

        LARGE_INTEGER zero = { 0 };
        SetFilePointerEx(file_handle, zero, NULL, FILE_BEGIN);
        SetEndOfFile(file_handle);
        success = MiniDumpWriteDump(process_handle, ctx->crashed_pid,
            file_handle, dump_type, &exception_info, NULL, NULL);
        error = GetLastError();
        SENTRY_WARNF("PROBE fallback: ok=%d error=%lu", success, error);
    }

    CloseHandle(process_handle);
    CloseHandle(file_handle);

    if (!success) {
        SENTRY_WARNF("MiniDumpWriteDump failed: %lu", error);
        probe_failure(ctx, output_path, dump_type);
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
