extern "C" {
#include "sentry_boot.h"
#include "sentry_core.h"
}

#include <ucontext.h>

#include <cstdint>
#include <cstdlib>
#include <dlfcn.h>
#include <memory>
#include <string>
#include <sys/system_properties.h>
#include <unistd.h>
#include <vector>

struct FrameData {
    size_t num;

    uint64_t rel_pc;
    uint64_t pc;
    uint64_t sp;

    std::string function_name;
    uint64_t function_offset = 0;

    std::string map_name;
    // The offset from the first map representing the frame. When there are
    // two maps (read-only and read-execute) this will be the offset from
    // the read-only map. When there is only one map, this will be the
    // same as the actual offset of the map and match map_exact_offset.
    uint64_t map_elf_start_offset = 0;
    // The actual offset from the map where the pc lies.
    uint64_t map_exact_offset = 0;
    uint64_t map_start = 0;
    uint64_t map_end = 0;
    uint64_t map_load_bias = 0;
    int map_flags = 0;
};

struct MapInfo {
    MapInfo(MapInfo *prev_map, MapInfo *prev_real_map, uint64_t start,
        uint64_t end, uint64_t offset, uint64_t flags, const char *name);

    MapInfo(MapInfo *prev_map, MapInfo *prev_real_map, uint64_t start,
        uint64_t end, uint64_t offset, uint64_t flags, const std::string &name);

    virtual ~MapInfo();
};

class Maps {
public:
    virtual ~Maps();

    Maps();

    Maps(const Maps &);

    Maps &operator=(const Maps &);

    Maps(Maps &&);

    Maps &operator=(Maps &&);

    virtual bool Parse();
};

class RemoteMaps : public Maps {
public:
    RemoteMaps(pid_t pid);

    virtual ~RemoteMaps();
};

class LocalMaps : public RemoteMaps {
public:
    LocalMaps()
        : RemoteMaps(getpid())
    {
    }

    virtual ~LocalMaps() = default;
};

struct Regs {
    virtual uint64_t pc() = 0;
};

enum ArchEnum : uint8_t {
    ARCH_UNKNOWN = 0,
    ARCH_ARM,
    ARCH_ARM64,
    ARCH_X86,
    ARCH_X86_64,
};

class Memory;

class Unwinder {
public:
    Unwinder(size_t max_frames, Maps *maps, Regs *regs,
        std::shared_ptr<Memory> process_memory);

    virtual ~Unwinder();

    virtual void Unwind(
        const std::vector<std::string> *initial_map_names_to_skip,
        const std::vector<std::string> *map_suffixes_to_ignore)
        = 0;

    virtual std::vector<FrameData> &frames() = 0;
};

typedef Regs *(*create_from_ucontext_t)(ArchEnum arch, void *ucontext);

typedef ArchEnum (*current_arch_t)();

typedef Regs *(*create_from_local_t)();

typedef std::shared_ptr<Memory> (*create_process_memory_cached_t)(pid_t);

static create_from_ucontext_t create_from_ucontext;
static current_arch_t current_arch;
static create_from_local_t create_from_local;
static create_process_memory_cached_t create_process_memory_cached;
static void *handle;

static bool
is_unwinder_loaded()
{
    return create_from_ucontext != nullptr && create_from_local != nullptr
        && current_arch != nullptr && create_process_memory_cached != nullptr;
}

static int
load_unwinder_symbols()
{
    create_from_ucontext = (create_from_ucontext_t)dlsym(
        handle, "_ZN11unwindstack4Regs18CreateFromUcontextENS_8ArchEnumEPv");
    if (!create_from_ucontext) {
        return EXIT_FAILURE;
    }
    current_arch
        = (current_arch_t)dlsym(handle, "_ZN11unwindstack4Regs11CurrentArchEv");
    if (!current_arch) {
        return EXIT_FAILURE;
    }
    create_from_local = (create_from_local_t)dlsym(
        handle, "_ZN11unwindstack4Regs15CreateFromLocalEv");
    if (!create_from_local) {
        return EXIT_FAILURE;
    }
    create_process_memory_cached = (create_process_memory_cached_t)dlsym(
        handle, "_ZN11unwindstack6Memory25CreateProcessMemoryCachedEi");
    if (!create_process_memory_cached) {
        return EXIT_FAILURE;
    }
}

extern "C" {
size_t
sentry__unwind_stack_libunwindstack(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (is_unwinder_loaded()) {
        Regs *regs = create_from_ucontext(current_arch(), uctx->user_context);
        LocalMaps maps;
        if (!maps.Parse()) {
            ptrs[0] = (void *)regs->pc();
            return 1;
        }

        const std::shared_ptr<Memory> process_memory
            = create_process_memory_cached(getpid());

        Unwinder unwinder(max_frames, &maps, regs, process_memory);
        unwinder.Unwind();

        std::vector<FrameData> &frames = unwinder.frames();

        size_t rv = 0;
        for (FrameData &frame : frames) {
            ptrs[rv++] = (void *)frame.pc;
        }

        delete regs;

        return rv;
    } else {
        SENTRY_WARN("cannot unwind: unwinder not fully loaded");
    }
    return 0;
}

void
sentry__load_unwinder()
{
    char sdk_ver_str[16];
    int sdk_ver;
    if (__system_property_get("ro.build.version.sdk", sdk_ver_str)) {
        char *end;
        sdk_ver = strtol(sdk_ver_str, &end, 10);
        if (sdk_ver_str == end) {
            SENTRY_WARN(
                "failed to read android SDK version to choose unwinder");
            return;
        }
    } else {
        SENTRY_WARN(
            "failed to find android system property \"ro.build.version.sdk\"");
        return;
    }

    if (sdk_ver >= 28) {
        SENTRY_DEBUG("running on android >= 28; load OS unwinder");
        handle = dlopen("libunwindstack.so", RTLD_LOCAL | RTLD_LAZY);
    } else {
        SENTRY_DEBUG("running on android < 28; load legacy unwinder");
        handle = dlopen("libandroid_unwinder_legacy.so", RTLD_LOCAL | RTLD_NOW);
    }
    if (!handle) {
        SENTRY_WARN("failed to load unwinder library");
        return;
    }
    if (load_unwinder_symbols() == EXIT_FAILURE) {
        SENTRY_WARN("failed to load unwinder symbols");
        return;
    }
}

void
sentry__unload_unwinder()
{
    if (handle == nullptr) {
        return;
    }
    int result = dlclose(handle);
    if (result) {
        SENTRY_WARN(dlerror());
    }
    create_from_ucontext = nullptr;
    create_from_local = nullptr;
    current_arch = nullptr;
    create_process_memory_cached = nullptr;
}
}
