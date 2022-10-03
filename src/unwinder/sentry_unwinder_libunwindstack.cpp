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

enum ArchEnum : uint8_t {
    ARCH_UNKNOWN = 0,
    ARCH_ARM,
    ARCH_ARM64,
    ARCH_X86,
    ARCH_X86_64,
};

class Memory;
enum ErrorCode : uint8_t {
    ERROR_NONE, // No error.
    ERROR_MEMORY_INVALID, // Memory read failed.
    ERROR_UNWIND_INFO, // Unable to use unwind information to unwind.
    ERROR_UNSUPPORTED, // Encountered unsupported feature.
    ERROR_INVALID_MAP, // Unwind in an invalid map.
    ERROR_MAX_FRAMES_EXCEEDED, // The number of frames exceed the total allowed.
    ERROR_REPEATED_FRAME, // The last frame has the same pc/sp as the next.
    ERROR_INVALID_ELF, // Unwind in an invalid elf.
    ERROR_THREAD_DOES_NOT_EXIST, // Attempt to unwind a local thread that does
    // not exist.
    ERROR_THREAD_TIMEOUT, // Timeout trying to unwind a local thread.
    ERROR_SYSTEM_CALL, // System call failed while unwinding.
};
struct ErrorData {
    ErrorCode code;
    uint64_t address; // Only valid when code is ERROR_MEMORY_INVALID.
    // Indicates the failing address.
};

typedef void *(*create_from_ucontext_t)(ArchEnum arch, void *ucontext);

typedef ArchEnum (*current_arch_t)();

typedef void *(*create_from_local_t)();
typedef void *(*local_maps_ctor_t)(void *);
typedef bool (*maps_parse_t)(void *);
typedef void *(*regs_pc_t)(void *);
typedef void *(*unwinder_ctor_t)(void *);
typedef void (*unwinder_unwind_t)(void *, void *, void *);
typedef std::vector<FrameData> &(*unwinder_frames_t)(void *);
typedef std::shared_ptr<Memory> (*create_process_memory_cached_t)(pid_t);

static create_from_ucontext_t create_from_ucontext = nullptr;
static current_arch_t current_arch = nullptr;
static create_from_local_t create_from_local = nullptr;
static create_process_memory_cached_t create_process_memory_cached = nullptr;
static local_maps_ctor_t local_maps_ctor = nullptr;
static maps_parse_t maps_parse = nullptr;
static regs_pc_t regs_pc = nullptr;
static unwinder_ctor_t unwinder_ctor = nullptr;
static unwinder_unwind_t unwinder_unwind = nullptr;
static unwinder_frames_t unwinder_frames = nullptr;
static void *handle = nullptr;

struct unwinder_data {
    unwinder_data(size_t max_frames, void *maps, void *regs,
        std::shared_ptr<Memory> process_memory)
        : max_frames_(max_frames)
        , maps_(maps)
        , regs_(regs)
        , process_memory_(process_memory)
        , arch_(current_arch())
    {
    }

    size_t max_frames_;
    void *maps_;
    void *regs_;
    std::vector<FrameData> frames_;
    std::shared_ptr<Memory> process_memory_;
    void *jit_debug_ = nullptr;
    void *dex_files_ = nullptr;
    bool resolve_names_ = true;
    bool embedded_soname_ = true;
    bool display_build_id_ = false;
    // True if at least one elf file is coming from memory and not the related
    // file. This is only true if there is an actual file backing up the elf.
    bool elf_from_memory_not_file_ = false;
    ErrorData last_error_ = { ERROR_NONE, 0 };
    uint64_t warnings_ = 0;
    ArchEnum arch_ = ARCH_UNKNOWN;
};

static bool
is_unwinder_loaded()
{
    return create_from_ucontext && current_arch && create_from_local
        && create_process_memory_cached && local_maps_ctor && maps_parse
        && regs_pc && unwinder_ctor && unwinder_unwind && unwinder_frames
        && handle;
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

    local_maps_ctor
        = (local_maps_ctor_t)dlsym(handle, "_ZTVN11unwindstack9LocalMapsE");
    if (!local_maps_ctor) {
        return EXIT_FAILURE;
    }

    maps_parse = (maps_parse_t)dlsym(handle, "_ZN11unwindstack4Maps5ParseEv");
    if (!maps_parse) {
        return EXIT_FAILURE;
    }

#if defined(__arm__)
    regs_pc = (regs_pc_t)dlsym(handle, "_ZN11unwindstack7RegsArm2pcEv");
#elif defined(__aarch64__)
    regs_pc = (regs_pc_t)dlsym(handle, "_ZN11unwindstack9RegsArm642pcEv");
#elif defined(__i386__)
    regs_pc = (regs_pc_t)dlsym(handle, "_ZN11unwindstack7RegsX862pcEv");
#elif defined(__x86_64__)
    regs_pc = (regs_pc_t)dlsym(handle, "_ZN11unwindstack10RegsX86_642pcEv");
#endif
    if (!regs_pc) {
        return EXIT_FAILURE;
    }

    unwinder_unwind = (unwinder_unwind_t)dlsym(handle,
        "_ZN11unwindstack8Unwinder6UnwindEPKNSt3__16vectorINS1_12basic_"
        "stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEENS6_IS8_EEEESC_");
    if (!unwinder_unwind) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

extern "C" {
size_t
sentry__unwind_stack_libunwindstack(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (is_unwinder_loaded()) {
        SENTRY_DEBUG("create_from_ucontext");

        void *regs = create_from_ucontext(current_arch(), uctx->user_context);
        SENTRY_DEBUG("allocate maps data");
        void *maps_data = malloc(32); // std::vector + pid_t
        SENTRY_DEBUG("construct LocalMaps");
        local_maps_ctor(maps_data);
        SENTRY_DEBUG("parse LocalMaps");
        if (!maps_parse(maps_data)) {
            SENTRY_DEBUG("retrieve PC from regs");
            ptrs[0] = regs_pc(regs);
            return 1;
        }

        SENTRY_DEBUG("create_process_memory_cached");
        const std::shared_ptr<Memory> process_memory
            = create_process_memory_cached(getpid());

        SENTRY_DEBUG("construct unwinder");
        auto *unwinder_data = new struct unwinder_data(
            max_frames, maps_data, regs, process_memory);
        SENTRY_DEBUG("unwind");
        unwinder_unwind(unwinder_data, nullptr, nullptr);

        size_t rv = 0;
        SENTRY_DEBUG("iterate stack frames");
        for (const FrameData &frame : unwinder_data->frames_) {
            SENTRY_DEBUG("store frame pointer");
            ptrs[rv++] = (void *)frame.pc;
        }

        delete unwinder_data;
        free(maps_data);
        free(regs);

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
            SENTRY_ERROR(
                "failed to read android SDK version to choose unwinder");
            return;
        }
    } else {
        SENTRY_ERROR(
            "failed to find android system property \"ro.build.version.sdk\"");
        return;
    }

    if (sdk_ver >= 28) {
        SENTRY_DEBUG("running on android >= 28; load OS unwinder");
        handle
            = dlopen("/system/lib64/libunwindstack.so", RTLD_LOCAL | RTLD_LAZY);
    } else {
        SENTRY_DEBUG("running on android < 28; load legacy unwinder");
        handle = dlopen("libandroid_unwinder_legacy.so", RTLD_LOCAL | RTLD_NOW);
    }
    if (!handle) {
        SENTRY_ERROR("failed to load unwinder library");
        SENTRY_ERROR(dlerror());
        return;
    }
    if (load_unwinder_symbols() == EXIT_FAILURE) {
        SENTRY_ERROR("failed to load unwinder symbols");
        SENTRY_ERROR(dlerror());
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
