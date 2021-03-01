extern "C" {
#include "sentry_boot.h"
#include "sentry_core.h"
#include "sentry_modulefinder_elf.h"
#include "sentry_sync.h"
#include "sentry_value.h"

#include <sys/mman.h>
}

#include <unwindstack/Elf.h>
#include <unwindstack/MapInfo.h>
#include <unwindstack/Maps.h>
#include <unwindstack/Memory.h>
#include <unwindstack/Regs.h>

extern "C" {

static bool g_initialized = false;
static sentry_mutex_t g_mutex = SENTRY__MUTEX_INIT;
static sentry_value_t g_modules = { 0 };

static void
load_modules(sentry_value_t modules)
{
    unwindstack::LocalMaps maps;
    if (!maps.Parse()) {
        SENTRY_WARN("libunwindstack failed to parse process maps");
        return;
    }

    unwindstack::ArchEnum arch = unwindstack::Regs::CurrentArch();
    const std::shared_ptr<unwindstack::Memory> process_memory
        = unwindstack::Memory::CreateProcessMemory(getpid());

    for (std::unique_ptr<unwindstack::MapInfo> &map : maps) {
        if (!(map->flags & (PROT_READ | PROT_EXEC))) {
            continue;
        }

        SENTRY_TRACEF("trying to get elf of mapping \"%s\"", map->name.c_str());
        unwindstack::Elf *elf = map->GetElf(process_memory, arch);
        unwindstack::Memory *memory = map->CreateMemory(process_memory);
        if (!elf || !elf->IsValidElf(memory)) {
            continue;
        }

        sentry_value_t mod_val = sentry_value_new_object();
        sentry_value_set_by_key(
            mod_val, "type", sentry_value_new_string("elf"));
        sentry_value_set_by_key(mod_val, "image_addr",
            sentry__value_new_addr((uint64_t)(size_t)map->start));
        sentry_value_set_by_key(mod_val, "image_size",
            sentry_value_new_int32(
                (int32_t)((size_t)map->end - (size_t)map->start)));
        sentry_value_set_by_key(
            mod_val, "code_file", sentry_value_new_string(map->name.c_str()));

        // sentry__procmaps_read_ids_from_elf(mod_val, (void
        // *)(size_t)map->start);

        const std::string &build_id = map->GetBuildID();
        SENTRY_TRACEF("got debug_ids: \"%s\" vs \"\"", build_id.c_str());

        sentry_value_append(modules, mod_val);
    }
}

sentry_value_t
sentry_get_modules_list(void)
{
    sentry__mutex_lock(&g_mutex);
    if (!g_initialized) {
        g_modules = sentry_value_new_list();
        load_modules(g_modules);
        sentry_value_freeze(g_modules);
        g_initialized = true;
    }
    sentry_value_t modules = g_modules;
    sentry_value_incref(modules);
    sentry__mutex_unlock(&g_mutex);
    return modules;
}

void
sentry_clear_modulecache(void)
{
    sentry__mutex_lock(&g_mutex);
    sentry_value_decref(g_modules);
    g_modules = sentry_value_new_null();
    g_initialized = false;
    sentry__mutex_unlock(&g_mutex);
}
}
