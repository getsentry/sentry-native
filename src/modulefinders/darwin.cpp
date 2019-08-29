#include "darwin.hpp"
#ifdef SENTRY_WITH_DARWIN_MODULE_FINDER

#include <dlfcn.h>
#include <mach-o/arch.h>
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>
#include <mach-o/loader.h>
#include <mach/mach_init.h>
#include <mach/mach_traps.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include "../uuid.hpp"

#if defined(GP_ARCH_x86)
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

static Value m_modules = Value();

void add_image(const mach_header *mh, intptr_t vmaddr_slide) {
    const platform_mach_header *header = (const platform_mach_header *)(mh);
    Dl_info info;
    if (!dladdr(header, &info)) {
        return;
    }

    Value modules = m_modules;
    Value new_modules = modules.is_null() ? Value::new_list() : modules.clone();
    Value module = Value::new_object();
    char buf[100];
    sprintf(buf, "0x%llx", (long long)info.dli_fbase);
    module.set_by_key("name", Value::new_string(info.dli_fname));
    module.set_by_key("image_addr", Value::new_string(buf));

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
            sentry_uuid_as_string(&uuid, buf);
            module.set_by_key("debug_id", Value::new_string(buf));
            has_uuid = true;
        }
    }

    new_modules.append(module);
    m_modules = new_modules;
}

void remove_image(const mach_header *mh, intptr_t vmaddr_slide) {
    const platform_mach_header *header = (const platform_mach_header *)(mh);
    Dl_info info;
    if (!dladdr(header, &info)) {
        return;
    }
}

DarwinModuleFinder::DarwinModuleFinder() {
    _dyld_register_func_for_add_image(add_image);
    _dyld_register_func_for_remove_image(remove_image);
}

Value DarwinModuleFinder::get_module_list() const {
    Value rv = m_modules;
    return rv.is_null() ? Value::new_list() : rv;
}

#endif
