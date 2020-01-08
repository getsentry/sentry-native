#include "sentry_windows_dbghelp.h"

#include "../sentry_sync.h"

#include <dbghelp.h>
#include <malloc.h>

static sentry_mutex_t g_sym_mutex = SENTRY__MUTEX_INIT;
static bool g_initialized;
static HANDLE g_proc = INVALID_HANDLE_VALUE;

#define MAX_SYM 1024

static void
init_dbghelp(void)
{
    sentry__mutex_lock(&g_sym_mutex);
    if (!g_initialized) {
        DWORD options = SymGetOptions();
        SymSetOptions(options | SYMOPT_UNDNAME);
        g_proc = GetCurrentProcess();
        SymInitialize(g_proc, NULL, TRUE);
        g_initialized = true;
    }
    sentry__mutex_unlock(&g_sym_mutex);
}

bool
sentry__symbolize_dbghelp(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data)
{
    init_dbghelp();

    SYMBOL_INFO *sym = (SYMBOL_INFO *)_alloca(sizeof(SYMBOL_INFO) + MAX_SYM);
    memset(sym, 0, sizeof(SYMBOL_INFO) + MAX_SYM);
    sym->MaxNameLen = MAX_SYM;
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);

    if (!SymFromAddr(g_proc, (DWORD64)addr, 0, sym)) {
        return false;
    }

    char mod_name[MAX_PATH];
    GetModuleFileNameA((HMODULE)sym->ModBase, mod_name, sizeof(mod_name));

    sentry_frame_info_t frame_info;
    memset(&frame_info, 0, sizeof(sentry_frame_info_t));
    frame_info.load_addr = (void *)sym->ModBase;
    frame_info.instruction_addr = addr;
    frame_info.symbol_addr = (void *)sym->Address;
    frame_info.symbol = sym->Name;
    frame_info.object_name = mod_name;
    func(&frame_info, data);

    return true;
}

size_t
sentry__unwind_stack_dbghelp(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (!uctx && !addr) {
        return (size_t)CaptureStackBackTrace(1, (ULONG)max_frames, ptrs, 0);
    }

    init_dbghelp();

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
