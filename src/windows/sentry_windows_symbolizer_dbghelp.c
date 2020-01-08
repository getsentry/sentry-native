#include "sentry_windows_symbolizer_dbghelp.h"

#include "../sentry_sync.h"

#include <dbghelp.h>
#include <malloc.h>

static sentry_mutex_t g_sym_mutex = SENTRY__MUTEX_INIT;
static bool g_initialized;
static HANDLE g_proc = INVALID_HANDLE_VALUE;

#define MAX_SYM 1024

bool
sentry__symbolize_dbghelp(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data)
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

    SYMBOL_INFO *sym = (SYMBOL_INFO *)_alloca(sizeof(SYMBOL_INFO) + MAX_SYM);
    memset(sym, 0, sizeof(SYMBOL_INFO) + MAX_SYM);
    sym->MaxNameLen = MAX_SYM;
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);

    if (!SymFromAddr(g_proc, (DWORD64)addr, 0, sym)) {
        return false;
    }

    char mod_name[MAX_PATH];
    HMODULE mod
        = GetModuleFileNameA((HMODULE)sym->ModBase, mod_name, sizeof(mod_name));

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