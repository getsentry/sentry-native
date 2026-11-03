extern "C" {
#include "sentry_boot.h"
#include "sentry_logger.h"
#include "sentry_unwinder.h"
}

#include <cstdio>
#include <cstring>
#include <memory>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <unwindstack/Maps.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>
#include <unwindstack/Unwinder.h>

static void
capture_registers(unwindstack::Regs *regs, sentry_remote_registers_t *registers)
{
    regs->IterateRegisters([registers](const char *name, uint64_t value) {
        size_t len = strlen(name);
        if (registers->count >= SENTRY_REMOTE_UNWIND_MAX_REGISTERS || len == 0
            || len >= sizeof(registers->values[0].name)) {
            return;
        }
        sentry_remote_register_t *remote_register
            = &registers->values[registers->count++];
        memcpy(remote_register->name, name, len + 1);
        remote_register->value = value;
    });
}

static size_t
unwind_thread(pid_t tid, sentry_remote_frame_t *frames, size_t max_frames,
    sentry_remote_registers_t *registers, bool symbolize)
{
    // an uncaught exception, e.g. std::bad_alloc, would abort the daemon
    try {
        std::unique_ptr<unwindstack::Regs> regs(
            unwindstack::Regs::RemoteGet(tid));
        if (!regs) {
            SENTRY_WARNF("libunwindstack failed to read registers of %d", tid);
            return 0;
        }
        if (registers) {
            capture_registers(regs.get(), registers);
        }

        unwindstack::RemoteMaps maps(tid);
        if (!maps.Parse()) {
            SENTRY_WARNF("libunwindstack failed to parse /proc/%d/maps", tid);
            return 0;
        }

        std::shared_ptr<unwindstack::Memory> process_memory
            = unwindstack::Memory::CreateProcessMemoryCached(tid);
        if (!process_memory) {
            SENTRY_WARNF(
                "libunwindstack failed to read process memory for %d", tid);
            return 0;
        }

        unwindstack::Unwinder unwinder(
            max_frames, &maps, regs.get(), process_memory);
        unwinder.SetResolveNames(symbolize);
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
            frames[count].ip = frame.pc;
            frames[count].symbol[0] = '\0';
            frames[count].symbol_offset = 0;
            if (!frame.function_name.empty()) {
                snprintf(frames[count].symbol, sizeof(frames[count].symbol),
                    "%s", frame.function_name.c_str());
                frames[count].symbol_offset = frame.function_offset;
            }
            count++;
        }

        SENTRY_DEBUGF(
            "libunwindstack remote unwind captured %zu frames", count);
        return count;
    } catch (...) {
        SENTRY_WARN("libunwindstack failed with an exception");
        return 0;
    }
}

extern "C" size_t
sentry__unwind_stack_from_thread_libunwindstack_remote(pid_t tid,
    sentry_remote_frame_t *frames, size_t max_frames,
    sentry_remote_registers_t *registers, bool symbolize)
{
    if (registers) {
        registers->count = 0;
    }

    if (tid <= 0 || !frames || max_frames == 0) {
        return 0;
    }

    if (!sentry__ptrace_attach_thread(tid)) {
        return 0;
    }

    size_t count = unwind_thread(tid, frames, max_frames, registers, symbolize);

    ptrace(PTRACE_DETACH, tid, NULL, NULL);
    return count;
}
