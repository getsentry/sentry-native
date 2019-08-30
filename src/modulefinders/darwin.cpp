#include "darwin.hpp"
#ifdef SENTRY_WITH_DARWIN_MODULE_FINDER

#include <dlfcn.h>
#include <limits.h>
#include <mach-o/arch.h>
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>
#include <mach-o/loader.h>
#include <mach/mach_init.h>
#include <mach/mach_traps.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <mutex>
#include "../uuid.hpp"

#if UINTPTR_MAX == 0xffffffffULL
typedef mach_header platform_mach_header;
typedef segment_command mach_segment_command_type;
#define MACHO_MAGIC_NUMBER MH_MAGIC
#define CMD_SEGMENT LC_SEGMENT
#define seg_size uint32_t
#else
typedef mach_header_64 platform_mach_header;
typedef segment_command_64 mach_segment_command_type;
#define MACHO_MAGIC_NUMBER MH_MAGIC_64
#define CMD_SEGMENT LC_SEGMENT_64
#define seg_size uint64_t
#endif

using namespace sentry;
using namespace modulefinders;

static Value g_modules;
static std::mutex g_modules_mutex;
static bool g_initialized = false;

void add_image(const mach_header *mh, intptr_t vmaddr_slide) {
    std::lock_guard<std::mutex> _guard(g_modules_mutex);

    const platform_mach_header *header = (const platform_mach_header *)(mh);
    Dl_info info;
    if (!dladdr(header, &info)) {
        return;
    }

    Value modules = g_modules;
    Value new_modules = modules.is_null() ? Value::new_list() : modules.clone();
    Value module = Value::new_object();
    module.set_by_key("code_file", Value::new_string(info.dli_fname));
    module.set_by_key("image_addr", Value::new_addr((uint64_t)info.dli_fbase));

    const struct load_command *cmd = (const struct load_command *)(header + 1);
    bool has_size = false;
    bool has_uuid = false;
    uint64_t start = (uint64_t)header;

    for (unsigned int i = 0;
         cmd && (i < header->ncmds) && (!has_uuid || !has_size);
         ++i, cmd = (const struct load_command *)((const char *)cmd +
                                                  cmd->cmdsize)) {
        if (cmd->cmd == CMD_SEGMENT) {
            const mach_segment_command_type *seg =
                (const mach_segment_command_type *)cmd;

            if (strcmp(seg->segname, "__TEXT") == 0) {
                module.set_by_key("image_size",
                                  Value::new_int32((uint32_t)seg->vmsize));
                has_size = true;
            }
        } else if (cmd->cmd == LC_UUID) {
            const uuid_command *ucmd = (const uuid_command *)cmd;
            sentry_uuid_t uuid =
                sentry_uuid_from_bytes((const char *)ucmd->uuid);
            module.set_by_key("debug_id", Value::new_uuid(&uuid));
            has_uuid = true;
        }
    }

    new_modules.append(module);
    g_modules = new_modules;
}

void remove_image(const mach_header *mh, intptr_t vmaddr_slide) {
    std::lock_guard<std::mutex> _guard(g_modules_mutex);
    if (g_modules.is_null() || g_modules.length() == 0) {
        return;
    }

    const platform_mach_header *header = (const platform_mach_header *)(mh);
    Dl_info info;
    if (!dladdr(header, &info)) {
        return;
    }

    char ref_addr[100];
    sprintf(ref_addr, "0x%llx", (long long)info.dli_fbase);
    Value new_modules = Value::new_list();

    for (size_t i = 0; i < g_modules.length(); i++) {
        Value module = g_modules.get_by_index(i);
        const char *addr = module.get_by_key("image_addr").as_cstr();
        if (!addr || strcmp(addr, ref_addr) != 0) {
            new_modules.append(module);
        }
    }

    g_modules = new_modules;
}

DarwinModuleFinder::DarwinModuleFinder() {
    if (!g_initialized) {
        _dyld_register_func_for_add_image(add_image);
        _dyld_register_func_for_remove_image(remove_image);
        g_initialized = true;
    }
}

Value DarwinModuleFinder::get_module_list() const {
    std::lock_guard<std::mutex> _guard(g_modules_mutex);
    Value rv(g_modules);
    return rv.is_null() ? Value::new_list() : rv;
}

#endif
