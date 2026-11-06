#include "sentry_windows_dbghelp.h"

#include <dbghelp.h>

#if _WIN32_WINNT && _WIN32_WINNT < 0x0600
typedef WORD(NTAPI *RtlCaptureStackBackTraceProc)(DWORD FramesToSkip,
    DWORD FramesToCapture, PVOID *BackTrace, PDWORD BackTraceHash);
#endif

#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wmissing-prototypes"
#    pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

size_t
sentry__unwind_stack_dbghelp(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (!uctx && !addr) {
#if _WIN32_WINNT && _WIN32_WINNT < 0x0600
        HMODULE ntdll = NULL;
        RtlCaptureStackBackTraceProc proc = NULL;

        if (!(ntdll = LoadLibraryW(L"ntdll.dll"))) {
            return 0;
        }
        if (!(proc = (RtlCaptureStackBackTraceProc)GetProcAddress(
                  ntdll, "RtlCaptureStackBackTrace"))) {
            return 0;
        }

        // sum of frames to skip and frames to captures must be less than 63 for
        // XP/2003
        if (max_frames > 61) {
            max_frames = 61;
        }

        return (size_t)proc(1, (DWORD)max_frames, ptrs, 0);
#else
        return (size_t)CaptureStackBackTrace(1, (ULONG)max_frames, ptrs, 0);
#endif
    }

#if (defined(_MSC_VER) || defined(__clang__))                                  \
    && (defined(_M_AMD64) || defined(_M_ARM64))
    if (!addr) {
        // unwind tables without DbgHelp or symbol loading
        CONTEXT walk = *uctx->exception_ptrs.ContextRecord;
#    if defined(_M_ARM64)
        DWORD64 *ip = &walk.Pc;
        DWORD64 *sp = &walk.Sp;
#    else
        DWORD64 *ip = &walk.Rip;
        DWORD64 *sp = &walk.Rsp;
#    endif
        size_t size = 0;
        if (size < max_frames) {
            ptrs[size++] = (void *)(uintptr_t)*ip;
        } else {
            return 0;
        }
        while (size < max_frames) {
            DWORD64 control_pc = *ip;
#    if defined(_M_ARM64)
            // use the call instruction for lookup, keeping frame PCs unadjusted
            if (walk.ContextFlags & CONTEXT_UNWOUND_TO_CALL) {
                control_pc -= 4;
            }
            DWORD64 previous_ip = *ip;
#    endif
            DWORD64 image_base = 0;
            PRUNTIME_FUNCTION entry = NULL;
            __try {
                entry = RtlLookupFunctionEntry(control_pc, &image_base, NULL);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                break;
            }
            DWORD64 previous_sp = *sp;
            if (entry) {
                PVOID handler_data = NULL;
                DWORD64 establisher = 0;
                __try {
                    RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, control_pc,
                        entry, &walk, &handler_data, &establisher, NULL);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    break;
                }
            } else {
#    if defined(_M_ARM64)
                walk.Pc = walk.Lr;
                walk.ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
#    else
                DWORD64 ret = 0;
                SIZE_T read = 0;
                if (!ReadProcessMemory(GetCurrentProcess(), (void *)walk.Rsp,
                        &ret, sizeof(ret), &read)
                    || read != sizeof(ret)) {
                    break;
                }
                walk.Rip = ret;
                walk.Rsp += sizeof(ret);
#    endif
            }
            if (!*ip) {
                break;
            }
            if (*ip < 0x1000
#    if defined(_M_AMD64)
                || *ip > 0x00007FFFFFFFFFFFULL
#    endif
            ) {
                break;
            }
#    if defined(_M_ARM64)
            if (*sp < previous_sp
                || (*sp == previous_sp && *ip == previous_ip)) {
#    else
            if (*sp <= previous_sp) {
#    endif
                break;
            }
            ptrs[size++] = (void *)(uintptr_t)*ip;
        }
        return size;
    }
#endif

    sentry__init_dbghelp();

    CONTEXT ctx = *uctx->exception_ptrs.ContextRecord;
    STACKFRAME64 stack_frame;
    memset(&stack_frame, 0, sizeof(stack_frame));

    size_t size = 0;
#if defined(_M_X64)
    DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;
    stack_frame.AddrPC.Offset = ctx.Rip;
    stack_frame.AddrFrame.Offset = ctx.Rbp;
    stack_frame.AddrStack.Offset = ctx.Rsp;
#elif defined(_M_IX86)
    DWORD machine_type = IMAGE_FILE_MACHINE_I386;
    stack_frame.AddrPC.Offset = ctx.Eip;
    stack_frame.AddrFrame.Offset = ctx.Ebp;
    stack_frame.AddrStack.Offset = ctx.Esp;
#elif defined(_M_ARM64)
    DWORD machine_type = IMAGE_FILE_MACHINE_ARM64;
    stack_frame.AddrPC.Offset = ctx.Pc;
#    if defined(NONAMELESSUNION)
    stack_frame.AddrFrame.Offset = ctx.DUMMYUNIONNAME.DUMMYSTRUCTNAME.Fp;
#    else
    stack_frame.AddrFrame.Offset = ctx.Fp;
#    endif
    stack_frame.AddrStack.Offset = ctx.Sp;
#elif defined(_M_ARM)
    DWORD machine_type = IMAGE_FILE_MACHINE_ARM;
    stack_frame.AddrPC.Offset = ctx.Pc;
    stack_frame.AddrFrame.Offset = ctx.R11;
    stack_frame.AddrStack.Offset = ctx.Sp;
#else
#    error "Platform not supported!"
#endif
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    if (addr) {
        stack_frame.AddrPC.Offset = (DWORD64)addr;
    }

    while (StackWalk64(machine_type, GetCurrentProcess(), GetCurrentThread(),
               &stack_frame, &ctx, NULL, SymFunctionTableAccess64,
               SymGetModuleBase64, NULL)
        && size < max_frames) {
        ptrs[size++] = (void *)(size_t)stack_frame.AddrPC.Offset;
    }

    return size;
}

#ifdef __clang__
#    pragma clang diagnostic pop
#endif
