#include "../symbolize.hpp"
#ifdef SENTRY_WITH_WINDOWS_SYMBOLIZER
#include <mutex>

#include <dbghelp.h>
#include <malloc.h>

using namespace sentry;
using namespace symbolizers;

static std::mutex g_sym_mutex;
static bool g_initialized;
static HANDLE g_proc = INVALID_HANDLE_VALUE;
#define MAX_SYM 1024

bool symbolizers::symbolize(void *addr,
                            std::function<void(const FrameInfo *)> func) {
    std::lock_guard<std::mutex> _guard(g_sym_mutex);
    if (!g_initialized) {
        DWORD options = SymGetOptions();
        SymSetOptions(options | SYMOPT_UNDNAME);
        g_proc = GetCurrentProcess();
        SymInitialize(g_proc, nullptr, true);
        g_initialized = true;
    }

    SYMBOL_INFO *sym = (SYMBOL_INFO *)_alloca(sizeof(SYMBOL_INFO) + MAX_SYM);
    memset(sym, 0, sizeof(SYMBOL_INFO) + MAX_SYM);
    sym->MaxNameLen = MAX_SYM;
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);

    if (!SymFromAddr(g_proc, (DWORD64)addr, 0, sym)) {
        return false;
    }

    FrameInfo frame_info = {0};
    frame_info.load_addr = (void *)sym->ModBase;
    frame_info.instruction_addr = addr;
    frame_info.symbol_addr = (void *)sym->Address;
    frame_info.symbol = sym->Name;
    func(&frame_info);

    return true;
}

#endif
