extern "C" {
#include <sentry.h>
}

#include <memory>
#include <ucontext.h>
#include <unwind.h>

extern "C" {

struct trace_state {
    void **current;
    void **end;
};

static _Unwind_Reason_Code
unwind_callback(struct _Unwind_Context *context, void *arg)
{
    auto *state = static_cast<trace_state *>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = reinterpret_cast<void *>(pc);
        }
    }
    return _URC_NO_REASON;
}

__attribute__((visibility("default"))) size_t
android_unwind_stack(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    (void)addr;
    (void)uctx;
    trace_state state = { ptrs, ptrs + max_frames };
    _Unwind_Backtrace(unwind_callback, &state);

    return state.current - ptrs;
}
}
