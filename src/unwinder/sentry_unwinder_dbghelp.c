#include "sentry_windows_dbghelp.h"

#include <dbghelp.h>

size_t
sentry__unwind_stack_dbghelp(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (!uctx && !addr) {
        return (size_t)CaptureStackBackTrace(1, (ULONG)max_frames, ptrs, 0);
    }

    sentry__init_dbghelp();

    CONTEXT *ctx = uctx->exception_ptrs.ContextRecord;
    STACKFRAME64 stack_frame;
    memset(&stack_frame, 0, sizeof(stack_frame));

    size_t size = 0;
#if defined(_WIN64)
    int machine_type = IMAGE_FILE_MACHINE_AMD64;
    stack_frame.AddrPC.Offset = ctx->Rip;
    stack_frame.AddrFrame.Offset = ctx->Rbp;
    stack_frame.AddrStack.Offset = ctx->Rsp;
#else
    int machine_type = IMAGE_FILE_MACHINE_I386;
    stack_frame.AddrPC.Offset = ctx->Eip;
    stack_frame.AddrFrame.Offset = ctx->Ebp;
    stack_frame.AddrStack.Offset = ctx->Esp;
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
        ptrs[size++] = (void *)stack_frame.AddrPC.Offset;
    }

    return size;
}
