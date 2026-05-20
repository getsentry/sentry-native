#include "sentry_crash_unwind.h"

#if defined(SENTRY_PLATFORM_ANDROID)                                           \
    && defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK)

extern "C" {
#    include "sentry_logger.h"
}

#    include <memory>
#    include <unwindstack/Maps.h>
#    include <unwindstack/Memory.h>
#    include <unwindstack/Regs.h>
#    include <unwindstack/Unwinder.h>

extern "C" size_t
sentry__crash_unwind_stack_libunwindstack(
    pid_t pid, const ucontext_t *uctx, uint64_t *ips, size_t max_frames)
{
    if (pid <= 0 || !uctx || !ips || max_frames == 0) {
        return 0;
    }

    std::unique_ptr<unwindstack::Regs> regs(
        unwindstack::Regs::CreateFromUcontext(
            unwindstack::Regs::CurrentArch(), const_cast<ucontext_t *>(uctx)));
    if (!regs) {
        SENTRY_WARN("libunwindstack failed to create registers from ucontext");
        return 0;
    }

    unwindstack::RemoteMaps maps(pid);
    if (!maps.Parse()) {
        SENTRY_WARNF("libunwindstack failed to parse /proc/%d/maps", pid);
        return 0;
    }

    std::shared_ptr<unwindstack::Memory> process_memory
        = unwindstack::Memory::CreateProcessMemoryCached(pid);
    if (!process_memory) {
        SENTRY_WARNF(
            "libunwindstack failed to read process memory for %d", pid);
        return 0;
    }

    unwindstack::Unwinder unwinder(
        max_frames, &maps, regs.get(), process_memory);
    unwinder.SetResolveNames(false);
    unwinder.Unwind();

    if (unwinder.LastErrorCode() != unwindstack::ERROR_NONE) {
        SENTRY_DEBUGF("libunwindstack finished with %s at 0x%llx",
            unwinder.LastErrorCodeString(),
            (unsigned long long)unwinder.LastErrorAddress());
    }

    size_t count = 0;
    for (const unwindstack::FrameData &frame : unwinder.frames()) {
        if (count >= max_frames) {
            break;
        }
        ips[count++] = frame.pc;
    }

    SENTRY_DEBUGF("libunwindstack remote unwind captured %zu frames", count);
    return count;
}

#endif
