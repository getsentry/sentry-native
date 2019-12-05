#include "../modulefinder.hpp"
#ifdef SENTRY_WITH_DARWIN_MODULEFINDER

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
static std::recursive_mutex g_modules_mutex;
static bool g_initialized = false;

/* there are potential issues with tearning down unloading a dylib on macos
   which causes the recursive mutex to fail with invalid_argument on attempted
   lock. We catch down this error here so we can recover from this situation
   without causing further issues.

   See GH-65 */
#define SAFE_LOCK_OR(Block)                                                  \
    do {                                                                     \
        std::unique_lock<std::recursive_mutex> _lock_guard(g_modules_mutex,  \
                                                           std::defer_lock); \
        try {                                                                \
            _lock_guard.lock();                                              \
        } catch (std::system_error) {                                        \
            Block;                                                           \
        }                                                                    \
    } while (0)

static void add_image(const mach_header *mh, intptr_t vmaddr_slide) {
    SAFE_LOCK_OR({ return; });

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

    module.set_by_key("type", Value::new_string("macho"));
    new_modules.append(module);
    new_modules.freeze();
    g_modules = new_modules;
}

static void remove_image(const mach_header *mh, intptr_t vmaddr_slide) {
    SAFE_LOCK_OR({ return; });

    if (g_modules.is_null() || g_modules.length() == 0) {
        return;
    }

    const platform_mach_header *header = (const platform_mach_header *)(mh);
    Dl_info info;
    if (!dladdr(header, &info)) {
        return;
    }

    char ref_addr[100];
    snprintf(ref_addr, sizeof(ref_addr), "0x%llx", (long long)info.dli_fbase);
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

Value modulefinders::get_module_list() {
    SAFE_LOCK_OR({ return Value::new_list(); });

    if (!g_initialized) {
        _dyld_register_func_for_add_image(add_image);
        _dyld_register_func_for_remove_image(remove_image);
        g_initialized = true;
    }

    return g_modules.is_null() ? Value::new_list() : g_modules;
}

#endif
