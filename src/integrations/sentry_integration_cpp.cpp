extern "C" {
#include "sentry_alloc.h"
#include "sentry_logger.h"
#include "sentry_slice.h"
#include "sentry_string.h"
}

#include "sentry_integration_cpp.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <new>
#include <typeinfo>

#if defined(__GXX_ABI_VERSION) && !defined(_MSC_VER)
#    include <cxxabi.h>
#endif

#if defined(__GXX_ABI_VERSION) && (defined(__ELF__) || defined(__APPLE__))
#    define SENTRY_CPP_THROW_INTERPOSE
#    include <dlfcn.h>
#    include <unwind.h>
#endif

#ifdef __clang__
#    define CLANG_DIAGNOSTIC_PUSH _Pragma("clang diagnostic push")
#    define CLANG_DIAGNOSTIC_IGNORE                                            \
        _Pragma("clang diagnostic ignored \"-Wlanguage-extension-token\"")
#    define CLANG_DIAGNOSTIC_POP _Pragma("clang diagnostic pop")
#else
#    define CLANG_DIAGNOSTIC_PUSH
#    define CLANG_DIAGNOSTIC_IGNORE
#    define CLANG_DIAGNOSTIC_POP
#endif

namespace {

static_assert(ATOMIC_POINTER_LOCK_FREE == 2,
    "SENTRY_INTEGRATION_CPP requires lock-free pointer atomics");
static_assert(ATOMIC_INT_LOCK_FREE == 2,
    "SENTRY_INTEGRATION_CPP requires lock-free integer atomics");

constexpr size_t TYPE_CAPACITY = 256;
constexpr size_t VALUE_CAPACITY = 1024;
constexpr size_t FRAME_CAPACITY = 128;
constexpr size_t THROW_CAPACITY = 8;

struct throw_trace_t {
    const void *object {};
    unsigned generation {};
    void *frames[FRAME_CAPACITY] {};
    size_t len {};
};

struct throw_state_t {
    throw_trace_t traces[THROW_CAPACITY] {};
    size_t next {};
    bool capturing {};
    bool terminating {};
};

thread_local throw_state_t g_throw_state {};
std::atomic<unsigned> g_cpp_generation { 0 };

enum metadata_state_t : unsigned {
    METADATA_EMPTY,
    METADATA_WRITING,
    METADATA_PENDING,
    METADATA_READING,
};

struct cpp_state_t {
    std::terminate_handler previous_handler {};
    std::atomic<metadata_state_t> metadata { METADATA_EMPTY };
    std::atomic_flag terminating = ATOMIC_FLAG_INIT;
    char type[TYPE_CAPACITY] {};
    char value[VALUE_CAPACITY] {};
    size_t type_len {};
    size_t value_len {};
    void *frames[FRAME_CAPACITY] {};
    size_t frame_count {};
    unsigned generation {};
    bool has_type {};
    bool has_value {};
    bool registered {};
#if defined(SENTRY_PLATFORM_WINDOWS) && defined(_MSC_VER)
    void *exception_handler {};
#endif
};

std::atomic<cpp_state_t *> g_cpp_state { nullptr };

void stash_metadata(
    cpp_state_t *data, const char *type, const char *value, const void *object);

#if defined(SENTRY_CPP_THROW_INTERPOSE)                                        \
    || (defined(SENTRY_PLATFORM_WINDOWS) && defined(_MSC_VER))
throw_trace_t *
begin_throw(const void *object)
{
    if (!g_cpp_state.load(std::memory_order_acquire) || g_throw_state.capturing
        || g_throw_state.terminating) {
        return nullptr;
    }

    g_throw_state.capturing = true;
    // replace reused exception addresses before consulting the cache
    for (auto &trace : g_throw_state.traces) {
        if (trace.object == object) {
            trace.object = nullptr;
        }
    }
    auto &trace = g_throw_state.traces[g_throw_state.next++ % THROW_CAPACITY];
    trace.object = object;
    trace.generation = g_cpp_generation.load(std::memory_order_relaxed);
    trace.len = 0;
    return &trace;
}
#endif

#ifdef SENTRY_CPP_THROW_INTERPOSE
struct throw_unwind_t {
    throw_trace_t *trace;
    void *caller;
};

_Unwind_Reason_Code
capture_frame(_Unwind_Context *context, void *_data)
{
    auto *data = static_cast<throw_unwind_t *>(_data);
    void *ip = reinterpret_cast<void *>(_Unwind_GetIP(context));
    if (!data->trace->len && ip != data->caller) {
        return _URC_NO_REASON;
    }
    if (!ip || data->trace->len == FRAME_CAPACITY) {
        return _URC_END_OF_STACK;
    }
    data->trace->frames[data->trace->len++] = ip;
    return _URC_NO_REASON;
}

void
capture_throw(void *object, void *caller)
{
#    ifdef __arm__
    // _Unwind_GetIP clears the Thumb bit
    caller = reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(caller) & ~static_cast<uintptr_t>(1));
#    endif
    if (auto *trace = begin_throw(object)) {
        throw_unwind_t data { trace, caller };
        _Unwind_Backtrace(capture_frame, &data);
        g_throw_state.capturing = false;
    }
}
#endif

#if defined(SENTRY_PLATFORM_WINDOWS) && defined(_MSC_VER)
// ehdata_values.h
constexpr DWORD MSVC_EH_EXCEPTION_NUMBER = 0xe06d7363;
constexpr ULONG_PTR MSVC_EH_MAGIC_NUMBER1 = 0x19930520;
constexpr ULONG_PTR MSVC_EH_MAGIC_NUMBER3 = 0x19930522;
constexpr ULONG_PTR MSVC_EH_PURE_MAGIC_NUMBER1 = 0x01994000;
constexpr DWORD MSVC_EH_EXCEPTION_PARAMETERS = sizeof(void *) == 8 ? 4 : 3;

// vcruntime's record of the exception being handled on this thread
extern "C" void **__cdecl __current_exception();

LONG CALLBACK
on_msvc_throw(EXCEPTION_POINTERS *ptrs)
{
    const auto *record = ptrs->ExceptionRecord;
    if (record->ExceptionCode == MSVC_EH_EXCEPTION_NUMBER
        && record->NumberParameters == MSVC_EH_EXCEPTION_PARAMETERS
        && (record->ExceptionInformation[0] == MSVC_EH_PURE_MAGIC_NUMBER1
            || (record->ExceptionInformation[0] >= MSVC_EH_MAGIC_NUMBER1
                && record->ExceptionInformation[0] <= MSVC_EH_MAGIC_NUMBER3))
        && record->ExceptionInformation[1] && record->ExceptionInformation[2]) {
        // x64 raises `throw;` again with the exception being handled
        const auto *current
            = static_cast<const EXCEPTION_RECORD *>(*__current_exception());
        if (current
            && current->ExceptionInformation[1]
                == record->ExceptionInformation[1]) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        if (auto *trace = begin_throw(
                reinterpret_cast<void *>(record->ExceptionInformation[1]))) {
            // avoid DbgHelp's process-global state in first-chance callbacks
            trace->len = CaptureStackBackTrace(
                1, static_cast<DWORD>(FRAME_CAPACITY), trace->frames, nullptr);
            g_throw_state.capturing = false;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

#    if defined(_WIN64)
using msvc_eh_data_ptr_t = int32_t;
#    else
using msvc_eh_data_ptr_t = uintptr_t;
#    endif

// ehdata_forceinclude.h (PMD)
struct msvc_pmd_t {
    int mdisp;
    int pdisp;
    int vdisp;
};

// ehdata_forceinclude.h (CatchableType)
struct msvc_catchable_type_t {
    unsigned properties;
    msvc_eh_data_ptr_t type;
    msvc_pmd_t displacement;
    int size_or_offset;
    msvc_eh_data_ptr_t copy_function;
};

// ehdata_forceinclude.h (CatchableTypeArray)
struct msvc_catchable_type_array_t {
    int count;
    msvc_eh_data_ptr_t types[1];
};

// ehdata_forceinclude.h (ThrowInfo)
struct msvc_throw_info_t {
    unsigned attributes;
    msvc_eh_data_ptr_t unwind_function;
    msvc_eh_data_ptr_t forward_compat;
    msvc_eh_data_ptr_t catchable_types;
};

// ehdata_forceinclude.h (TypeDescriptor)
struct msvc_type_descriptor_t {
    const void *vtable;
    void *spare;
    char name[1];
};

static_assert(sizeof(msvc_pmd_t) == 12, "unexpected MSVC PMD layout");
static_assert(sizeof(msvc_catchable_type_t) == 28,
    "unexpected MSVC CatchableType layout");
static_assert(
    sizeof(msvc_throw_info_t) == 16, "unexpected MSVC ThrowInfo layout");

const void *
resolve_exception_data(uintptr_t image_base, msvc_eh_data_ptr_t value)
{
#    if defined(_WIN64)
    if (!image_base || !value) {
        return nullptr;
    }
    return reinterpret_cast<const void *>(
        image_base + static_cast<uint32_t>(value));
#    else
    (void)image_base;
    return reinterpret_cast<const void *>(value);
#    endif
}

void *
adjust_exception_pointer(void *object, const msvc_pmd_t &displacement)
{
    auto *adjusted = static_cast<unsigned char *>(object) + displacement.mdisp;
    if (displacement.pdisp >= 0) {
        auto *vbptr = reinterpret_cast<unsigned char **>(
            static_cast<unsigned char *>(object) + displacement.pdisp);
        adjusted += displacement.pdisp
            + *reinterpret_cast<int *>(*vbptr + displacement.vdisp);
    }
    return adjusted;
}

void *
decode_std_exception(
    const EXCEPTION_RECORD *record, const char *std_exception_name)
{
    CLANG_DIAGNOSTIC_PUSH
    CLANG_DIAGNOSTIC_IGNORE
    __try {
        if (!record || record->ExceptionCode != MSVC_EH_EXCEPTION_NUMBER
            || record->NumberParameters != MSVC_EH_EXCEPTION_PARAMETERS
            || (record->ExceptionInformation[0] != MSVC_EH_PURE_MAGIC_NUMBER1
                && (record->ExceptionInformation[0] < MSVC_EH_MAGIC_NUMBER1
                    || record->ExceptionInformation[0]
                        > MSVC_EH_MAGIC_NUMBER3))) {
            return nullptr;
        }

        void *object
            = reinterpret_cast<void *>(record->ExceptionInformation[1]);
        uintptr_t image_base = sizeof(void *) == 8
            ? static_cast<uintptr_t>(record->ExceptionInformation[3])
            : 0;
        const auto *info = reinterpret_cast<const msvc_throw_info_t *>(
            record->ExceptionInformation[2]);
        if (!object || !info) {
            return nullptr;
        }

        const auto *types = static_cast<const msvc_catchable_type_array_t *>(
            resolve_exception_data(image_base, info->catchable_types));
        // Bound scans of corrupt metadata.
        if (!types || types->count <= 0 || types->count > 64) {
            return nullptr;
        }

        for (int i = 0; i < types->count; i++) {
            const auto *type = static_cast<const msvc_catchable_type_t *>(
                resolve_exception_data(image_base, types->types[i]));
            if (!type) {
                continue;
            }
            const auto *descriptor
                = static_cast<const msvc_type_descriptor_t *>(
                    resolve_exception_data(image_base, type->type));
            if (descriptor
                && sentry__string_eq(descriptor->name, std_exception_name)) {
                return adjust_exception_pointer(object, type->displacement);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    CLANG_DIAGNOSTIC_POP
    return nullptr;
}

void
stash_msvc_exception(cpp_state_t *data, const EXCEPTION_RECORD *record)
{
    const char *std_exception_name = typeid(std::exception).raw_name();
    auto *exception = static_cast<const std::exception *>(
        decode_std_exception(record, std_exception_name));
    if (!exception) {
        return;
    }

    const char *type = nullptr;
    const char *value = nullptr;
    try {
        type = typeid(*exception).name();
    } catch (...) {
    }
    try {
        value = exception->what();
    } catch (...) {
    }
    stash_metadata(data, type, value,
        reinterpret_cast<void *>(record->ExceptionInformation[1]));
}

#endif

size_t
copy_bounded(char *dst, size_t capacity, const char *src)
{
    if (!src || capacity == 0) {
        return 0;
    }

    sentry_slice_t slice = sentry__slice_from_str(src);
    if (slice.len >= capacity) {
        slice.len = capacity - 1;
    }
    sentry__slice_to_buffer(slice, dst, capacity);
    return slice.len;
}

void
reset_metadata(cpp_state_t *data)
{
    data->type[0] = '\0';
    data->value[0] = '\0';
    data->type_len = 0;
    data->value_len = 0;
    data->frame_count = 0;
    data->has_type = false;
    data->has_value = false;
    data->metadata.store(METADATA_EMPTY, std::memory_order_release);
}

void
stash_metadata(
    cpp_state_t *data, const char *type, const char *value, const void *object)
{
    if (!data || (!type && !value)) {
        return;
    }

    metadata_state_t expected = METADATA_EMPTY;
    if (!data->metadata.compare_exchange_strong(expected, METADATA_WRITING,
            std::memory_order_acquire, std::memory_order_relaxed)) {
        return;
    }

    data->frame_count = 0;
    if (!g_throw_state.capturing) {
        for (const auto &trace : g_throw_state.traces) {
            if (object && trace.object == object
                && trace.generation == data->generation) {
                memcpy(data->frames, trace.frames, trace.len * sizeof(void *));
                data->frame_count = trace.len;
                break;
            }
        }
    }

#if defined(__GXX_ABI_VERSION) && !defined(_MSC_VER)
    char *demangled
        = type ? abi::__cxa_demangle(type, nullptr, nullptr, nullptr) : nullptr;
    if (demangled) {
        type = demangled;
    }
#endif

    data->has_type = type != nullptr;
    data->has_value = value != nullptr;
    data->type_len = copy_bounded(data->type, sizeof(data->type), type);
    data->value_len = copy_bounded(data->value, sizeof(data->value), value);

#if defined(__GXX_ABI_VERSION) && !defined(_MSC_VER)
    std::free(demangled);
#endif

    data->metadata.store(METADATA_PENDING, std::memory_order_release);
}

[[noreturn]] void
terminate_handler() noexcept
{
    cpp_state_t *data = g_cpp_state.load(std::memory_order_acquire);
    if (!data || data->terminating.test_and_set(std::memory_order_acquire)) {
        std::abort();
    }

    g_throw_state.terminating = true;
    try {
        std::exception_ptr current = std::current_exception();
        if (current) {
            throw;
        }
    } catch (const std::exception &exception) {
        const char *type = nullptr;
        const char *value = nullptr;
        try {
            type = typeid(exception).name();
        } catch (...) {
        }
        try {
            value = exception.what();
        } catch (...) {
        }
        stash_metadata(
            data, type, value, dynamic_cast<const void *>(&exception));
    } catch (...) {
    }

    std::terminate_handler previous_handler = data->previous_handler;
    if (!previous_handler || previous_handler == terminate_handler) {
        std::abort();
    }
    previous_handler();
    std::abort();
}

void
register_cpp(void *_data, sentry_scope_t *, const sentry_options_t *)
{
    auto *data = static_cast<cpp_state_t *>(_data);
    reset_metadata(data);
    data->generation
        = g_cpp_generation.fetch_add(1, std::memory_order_relaxed) + 1;
    g_cpp_state.store(data, std::memory_order_release);
    data->previous_handler = std::set_terminate(terminate_handler);
#if defined(SENTRY_PLATFORM_WINDOWS) && defined(_MSC_VER)
    data->exception_handler = AddVectoredExceptionHandler(1, on_msvc_throw);
    if (!data->exception_handler) {
        SENTRY_WARN("failed to install C++ throw stack capture");
    }
#endif
    data->registered = true;
}

void
unregister_cpp(void *_data, sentry_scope_t *, const sentry_options_t *)
{
    auto *data = static_cast<cpp_state_t *>(_data);
    cpp_state_t *expected = data;
    g_cpp_state.compare_exchange_strong(
        expected, nullptr, std::memory_order_acq_rel);

#if defined(SENTRY_PLATFORM_WINDOWS) && defined(_MSC_VER)
    if (data->exception_handler) {
        RemoveVectoredExceptionHandler(data->exception_handler);
        data->exception_handler = nullptr;
    }
#endif
    if (data->registered && std::get_terminate() == terminate_handler) {
        std::set_terminate(data->previous_handler);
    }
    data->registered = false;
    data->previous_handler = nullptr;
    reset_metadata(data);
}

void
free_cpp(void *data)
{
    delete static_cast<cpp_state_t *>(data);
}

sentry_value_t
get_exception(sentry_value_t event)
{
    sentry_value_t exception = sentry_value_get_by_key(event, "exception");
    if (sentry_value_get_type(exception) == SENTRY_VALUE_TYPE_OBJECT) {
        exception = sentry_value_get_by_key(exception, "values");
    }
    if (sentry_value_get_type(exception) != SENTRY_VALUE_TYPE_LIST
        || sentry_value_get_length(exception) == 0) {
        return sentry_value_new_null();
    }
    return sentry_value_get_by_index(exception, 0);
}

void
on_cpp_crash(void *_data, const sentry_ucontext_t *uctx, sentry_value_t event)
{
    auto *data = static_cast<cpp_state_t *>(_data);

#ifdef SENTRY_PLATFORM_WINDOWS
    if (uctx && uctx->exception_ptrs.ExceptionRecord) {
        DWORD code = uctx->exception_ptrs.ExceptionRecord->ExceptionCode;
        bool is_cpp_crash = code == STATUS_FATAL_APP_EXIT;
#    if defined(_MSC_VER)
        if (code == MSVC_EH_EXCEPTION_NUMBER) {
            is_cpp_crash = true;
            if (data->metadata.load(std::memory_order_acquire)
                == METADATA_EMPTY) {
                stash_msvc_exception(
                    data, uctx->exception_ptrs.ExceptionRecord);
            }
        }
#    endif
        if (!is_cpp_crash) {
            reset_metadata(data);
            return;
        }
    }
#else
    (void)uctx;
#endif

    metadata_state_t expected = METADATA_PENDING;
    if (!data->metadata.compare_exchange_strong(expected, METADATA_READING,
            std::memory_order_acquire, std::memory_order_relaxed)) {
        return;
    }

    const char *type = data->has_type ? data->type : "C++ Exception";
    size_t type_len
        = data->has_type ? data->type_len : sizeof("C++ Exception") - 1;
    const char *value = data->has_value ? data->value : "";
    size_t value_len = data->has_value ? data->value_len : 0;

    sentry_value_t exception = get_exception(event);
    if (sentry_value_is_null(exception)) {
        exception
            = sentry_value_new_exception_n(type, type_len, value, value_len);
        sentry_event_add_exception(event, exception);
    } else {
        sentry_value_set_by_key(
            exception, "type", sentry_value_new_string_n(type, type_len));
        sentry_value_set_by_key(
            exception, "value", sentry_value_new_string_n(value, value_len));
    }
    if (data->frame_count) {
        sentry_value_t stacktrace
            = sentry_value_new_stacktrace(data->frames, data->frame_count);
        sentry_value_set_by_key(stacktrace, "instruction_addr_adjustment",
            sentry_value_new_string("all"));
        sentry_value_set_by_key(exception, "stacktrace", stacktrace);
    }
    sentry_value_t mechanism = sentry_value_get_by_key(exception, "mechanism");
    if (sentry_value_get_type(mechanism) != SENTRY_VALUE_TYPE_OBJECT) {
        mechanism = sentry_value_new_object();
        sentry_value_set_by_key(exception, "mechanism", mechanism);
    }
    sentry_value_set_by_key(
        mechanism, "type", sentry_value_new_string("cpp_exception"));
    sentry_value_set_by_key(mechanism, "handled", sentry_value_new_bool(false));
    if (data->frame_count) {
        sentry_value_t mechanism_data
            = sentry_value_get_by_key(mechanism, "data");
        if (sentry_value_get_type(mechanism_data) != SENTRY_VALUE_TYPE_OBJECT) {
            mechanism_data = sentry_value_new_object();
            sentry_value_set_by_key(mechanism, "data", mechanism_data);
        }
        sentry_value_set_by_key(
            mechanism_data, "throw_stacktrace", sentry_value_new_bool(true));
    }
    sentry_value_remove_by_key(mechanism, "synthetic");
    reset_metadata(data);
}

} // namespace

#ifdef SENTRY_CPP_THROW_INTERPOSE
using cxa_throw_t = void (*)(void *, void *, void (*)(void *));
using cxa_allocate_t = void *(*)(size_t);

#    ifdef __APPLE__
// dyld ignores weak replacements and leaves calls from this image as is
#        define SENTRY_CPP_WEAK
#        define SENTRY_CPP_SYMBOL(name)
#        define SENTRY_CPP_ORIGINAL(name) &abi::name
#    else
#        ifdef SENTRY_BUILD_SHARED
// a static C++ runtime must fail to link instead of being exported
#            define SENTRY_CPP_WEAK
#        else
#            define SENTRY_CPP_WEAK __attribute__((weak))
#        endif
#        define SENTRY_CPP_SYMBOL(name) __asm__(name)
#        define SENTRY_CPP_ORIGINAL(name) dlsym(RTLD_NEXT, #name)
#    endif

extern "C" {
SENTRY_CPP_WEAK __attribute__((visibility("default"), noinline))
[[noreturn]] void
sentry__cxa_throw(void *object, void *type, void (*destroy)(void *))
    SENTRY_CPP_SYMBOL("__cxa_throw");
SENTRY_CPP_WEAK __attribute__((visibility("default"))) void *
sentry__cxa_allocate_exception(size_t size) noexcept
    SENTRY_CPP_SYMBOL("__cxa_allocate_exception");

void
sentry__cxa_throw(void *object, void *type, void (*destroy)(void *))
{
    static auto original
        = reinterpret_cast<cxa_throw_t>(SENTRY_CPP_ORIGINAL(__cxa_throw));
    if (!original) {
        std::abort();
    }
    capture_throw(
        object, __builtin_extract_return_addr(__builtin_return_address(0)));
    original(object, type, destroy);
    std::abort();
}

void *
sentry__cxa_allocate_exception(size_t size) noexcept
{
    static auto original = reinterpret_cast<cxa_allocate_t>(
        SENTRY_CPP_ORIGINAL(__cxa_allocate_exception));
    if (!original) {
        std::abort();
    }
    void *object = original(size);
    // drop stale traces here because runtime-internal frees bypass hooks
    for (auto &trace : g_throw_state.traces) {
        if (trace.object == object) {
            trace.object = nullptr;
        }
    }
    return object;
}
}

#    ifdef __APPLE__
__attribute__((used, section("__DATA,__interpose"))) static const struct {
    const void *replacement;
    const void *original;
} g_cpp_interpose[] = {
    { reinterpret_cast<const void *>(sentry__cxa_throw),
        reinterpret_cast<const void *>(abi::__cxa_throw) },
    { reinterpret_cast<const void *>(sentry__cxa_allocate_exception),
        reinterpret_cast<const void *>(abi::__cxa_allocate_exception) },
};
#    endif
#endif

extern "C" sentry_integration_t *
sentry_integration_cpp_new(void)
{
    sentry_integration_t *integration = SENTRY_MAKE(sentry_integration_t);
    if (!integration) {
        return nullptr;
    }

    auto *data = new (std::nothrow) cpp_state_t {};
    if (!data) {
        sentry_free(integration);
        return nullptr;
    }

    integration->name = "cpp";
    integration->data = data;
    integration->register_func = register_cpp;
    integration->unregister_func = unregister_cpp;
    integration->free_func = free_cpp;
    integration->on_crash_func = on_cpp_crash;
    return integration;
}
