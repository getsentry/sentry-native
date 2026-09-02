extern "C" {
#include "sentry_alloc.h"
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
    bool has_type {};
    bool has_value {};
    bool registered {};
};

std::atomic<cpp_state_t *> g_cpp_state { nullptr };

void stash_metadata(cpp_state_t *data, const char *type, const char *value);

#if defined(SENTRY_PLATFORM_WINDOWS) && defined(_MSC_VER)
// ehdata_values.h
constexpr DWORD MSVC_EH_EXCEPTION_NUMBER = 0xe06d7363;
constexpr ULONG_PTR MSVC_EH_MAGIC_NUMBER1 = 0x19930520;
constexpr ULONG_PTR MSVC_EH_MAGIC_NUMBER3 = 0x19930522;
constexpr ULONG_PTR MSVC_EH_PURE_MAGIC_NUMBER1 = 0x01994000;
constexpr DWORD MSVC_EH_EXCEPTION_PARAMETERS = sizeof(void *) == 8 ? 4 : 3;

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
    stash_metadata(data, type, value);
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
    data->has_type = false;
    data->has_value = false;
    data->metadata.store(METADATA_EMPTY, std::memory_order_release);
}

void
stash_metadata(cpp_state_t *data, const char *type, const char *value)
{
    if (!data || (!type && !value)) {
        return;
    }

    metadata_state_t expected = METADATA_EMPTY;
    if (!data->metadata.compare_exchange_strong(expected, METADATA_WRITING,
            std::memory_order_acquire, std::memory_order_relaxed)) {
        return;
    }

    data->has_type = type != nullptr;
    data->has_value = value != nullptr;
    data->type_len = copy_bounded(data->type, sizeof(data->type), type);
    data->value_len = copy_bounded(data->value, sizeof(data->value), value);

    data->metadata.store(METADATA_PENDING, std::memory_order_release);
}

[[noreturn]] void
terminate_handler() noexcept
{
    cpp_state_t *data = g_cpp_state.load(std::memory_order_acquire);
    if (!data || data->terminating.test_and_set(std::memory_order_acquire)) {
        std::abort();
    }

    try {
        std::exception_ptr current = std::current_exception();
        if (current) {
            std::rethrow_exception(current);
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
        stash_metadata(data, type, value);
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
    g_cpp_state.store(data, std::memory_order_release);
    data->previous_handler = std::set_terminate(terminate_handler);
    data->registered = true;
}

void
unregister_cpp(void *_data, sentry_scope_t *, const sentry_options_t *)
{
    auto *data = static_cast<cpp_state_t *>(_data);
    cpp_state_t *expected = data;
    g_cpp_state.compare_exchange_strong(
        expected, nullptr, std::memory_order_acq_rel);

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

    char value[TYPE_CAPACITY + 2 + VALUE_CAPACITY];
    size_t value_len = 0;
    if (data->has_type) {
        memcpy(value, data->type, data->type_len);
        value_len = data->type_len;
    }
    if (data->has_type && data->has_value) {
        value[value_len++] = ':';
        value[value_len++] = ' ';
    }
    if (data->has_value) {
        memcpy(value + value_len, data->value, data->value_len);
        value_len += data->value_len;
    }
    value[value_len] = '\0';

    sentry_value_t exception = get_exception(event);
    if (sentry_value_is_null(exception)) {
        exception = sentry_value_new_exception_n(
            "C++ Exception", sizeof("C++ Exception") - 1, value, value_len);
        sentry_event_add_exception(event, exception);
    } else {
        sentry_value_set_by_key(
            exception, "type", sentry_value_new_string("C++ Exception"));
        sentry_value_set_by_key(
            exception, "value", sentry_value_new_string_n(value, value_len));
    }
    sentry_value_t mechanism = sentry_value_get_by_key(exception, "mechanism");
    if (sentry_value_get_type(mechanism) != SENTRY_VALUE_TYPE_OBJECT) {
        mechanism = sentry_value_new_object();
        sentry_value_set_by_key(exception, "mechanism", mechanism);
    }
    sentry_value_set_by_key(
        mechanism, "type", sentry_value_new_string("cpp_exception"));
    sentry_value_set_by_key(mechanism, "handled", sentry_value_new_bool(false));
    sentry_value_remove_by_key(mechanism, "synthetic");
    reset_metadata(data);
}

} // namespace

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
