#include "../unwind.hpp"
#ifdef SENTRY_WITH_LIBUNWINDSTACK_UNWINDER

#include <unwindstack/Elf.h>
#include <unwindstack/MapInfo.h>
#include <unwindstack/Maps.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>

using namespace sentry;

size_t unwinders::unwind_stack_libunwindstack(void *addr,
                                              void **ptrs,
                                              size_t max_frames) {
    std::unique_ptr<unwindstack::Regs> regs(
        unwindstack::Regs::CreateFromLocal());

    unwindstack::LocalMaps maps;
    if (!maps.Parse()) {
        ptrs[0] = (void *)regs->pc();
        return 1;
    }

    const std::shared_ptr<unwindstack::Memory> process_memory(
        new unwindstack::MemoryLocal);

    int rv = 0;
    for (int i = 0; i < max_frames; i++) {
        ptrs[rv++] = (void *)regs->pc();
        unwindstack::MapInfo *const map_info = maps.Find(regs->pc());
        if (!map_info) {
            break;
        }

        // the boolean false parameter disables debugdata loading which we don't
        // want due to size constraints.  Also that data is unlikely to be
        // useful anyways.
        unwindstack::Elf *const elf = map_info->GetElf(process_memory, false);
        if (!elf) {
            break;
        }

        uint64_t rel_pc = elf->GetRelPc(regs->pc(), map_info);
        uint64_t adjusted_rel_pc = rel_pc - regs->GetPcAdjustment(rel_pc, elf);
        bool finished = false;
        if (!elf->Step(rel_pc, adjusted_rel_pc, map_info->elf_offset,
                       regs.get(), process_memory.get(), &finished)) {
            break;
        }
    }

    return rv;
}

#endif
