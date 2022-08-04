extern "C" {
#include <sentry.h>
}

#include <memory>
#include <ucontext.h>
#include <unwindstack/Maps.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>
#include <unwindstack/RegsGetLocal.h>
#include <unwindstack/Unwinder.h>

extern "C" {

__attribute__((visibility("default"))) size_t
android_unwind_stack(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    std::unique_ptr<unwindstack::Regs> regs;

    if (uctx) {
        regs = std::unique_ptr<unwindstack::Regs>(
            unwindstack::Regs::CreateFromUcontext(
                unwindstack::Regs::CurrentArch(), uctx->user_context));
    } else if (!addr) {
        regs = std::unique_ptr<unwindstack::Regs>(
            unwindstack::Regs::CreateFromLocal());
        unwindstack::RegsGetLocal(regs.get());
    } else {
        return 0;
    }

    unwindstack::LocalMaps maps;
    if (!maps.Parse()) {
        ptrs[0] = (void *)regs->pc();
        return 1;
    }

    const std::shared_ptr<unwindstack::Memory> process_memory
        = unwindstack::Memory::CreateProcessMemoryCached(getpid());

    unwindstack::Unwinder unwinder(
        max_frames, &maps, regs.get(), process_memory);
    unwinder.Unwind();

    std::vector<unwindstack::FrameData> &frames = unwinder.frames();

    size_t rv = 0;
    for (unwindstack::FrameData &frame : frames) {
        ptrs[rv++] = (void *)frame.pc;
    }

    return rv;
}
}
