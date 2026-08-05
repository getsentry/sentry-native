#include "sentry_scope.h"
#include "sentry_alloc.h"
#include "sentry_attachment.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_options.h"
#include "sentry_os.h"
#include "sentry_ringbuffer.h"
#include "sentry_string.h"
#include "sentry_symbolizer.h"
#include "sentry_sync.h"
#include "sentry_tracing.h"
#include "sentry_uuid.h"
#include "sentry_value.h"

#include <stdlib.h>

#ifdef SENTRY_BACKEND_CRASHPAD
#    define SENTRY_BACKEND "crashpad"
#elif defined(SENTRY_BACKEND_BREAKPAD)
#    define SENTRY_BACKEND "breakpad"
#elif defined(SENTRY_BACKEND_INPROC)
#    define SENTRY_BACKEND "inproc"
#elif defined(SENTRY_BACKEND_NATIVE)
#    define SENTRY_BACKEND "native"
#endif

static bool g_scope_initialized = false;
static sentry_scope_t g_scope = { 0 };
#ifdef SENTRY__MUTEX_INIT_DYN
SENTRY__MUTEX_INIT_DYN(g_lock)
#else
static sentry_mutex_t g_lock = SENTRY__MUTEX_INIT;
#endif

#define SCOPE_READ_LOCK(Scope)                                                 \
    for (const sentry_scope_t *_locked_scope = (Scope); _locked_scope;         \
        sentry__rwlock_read_unlock((sentry_rwlock_t *)&_locked_scope->rwlock), \
                              _locked_scope = NULL)                            \
        for (bool _locked_once                                                 \
            = (sentry__rwlock_read_lock(                                       \
                   (sentry_rwlock_t *)&_locked_scope->rwlock),                 \
                true);                                                         \
            _locked_once; _locked_once = false)

#define SCOPE_WRITE_LOCK(Scope)                                                \
    for (sentry_scope_t *_locked_scope = (Scope); _locked_scope;               \
        sentry__rwlock_write_unlock(&_locked_scope->rwlock),                   \
                        _locked_scope = NULL)                                  \
        for (bool _locked_once                                                 \
            = (sentry__rwlock_write_lock(&_locked_scope->rwlock), true);       \
            _locked_once; _locked_once = false)

static sentry_value_t
get_client_sdk(void)
{
    sentry_value_t client_sdk = sentry_value_new_object();

    // the SDK is not initialized yet, fallback to build-time value
    sentry_value_t sdk_name = sentry_value_new_string(SENTRY_SDK_NAME);
    sentry_value_set_by_key(client_sdk, "name", sdk_name);

    sentry_value_t version = sentry_value_new_string(SENTRY_SDK_VERSION);
    sentry_value_set_by_key(client_sdk, "version", version);

    sentry_value_t package = sentry_value_new_object();

    sentry_value_t package_name
        = sentry_value_new_string("github:getsentry/sentry-native");
    sentry_value_set_by_key(package, "name", package_name);

    sentry_value_incref(version);
    sentry_value_set_by_key(package, "version", version);

    sentry_value_t packages = sentry_value_new_list();
    sentry_value_append(packages, package);
    sentry_value_set_by_key(client_sdk, "packages", packages);

#ifdef SENTRY_BACKEND
    sentry_value_t integrations = sentry_value_new_list();
    sentry_value_append(integrations, sentry_value_new_string(SENTRY_BACKEND));
    sentry_value_set_by_key(client_sdk, "integrations", integrations);
#endif

    return client_sdk;
}

static void
generate_propagation_context(sentry_value_t propagation_context)
{
    sentry_value_set_by_key(
        propagation_context, "trace", sentry_value_new_object());
    sentry_uuid_t trace_id = sentry_uuid_new_v4();
    sentry_uuid_t span_id = sentry_uuid_new_v4();
    sentry_value_set_by_key(
        sentry_value_get_by_key(propagation_context, "trace"), "trace_id",
        sentry__value_new_internal_uuid(&trace_id));
    sentry_value_set_by_key(
        sentry_value_get_by_key(propagation_context, "trace"), "span_id",
        sentry__value_new_span_uuid(&span_id));
    sentry__generate_sample_rand(
        sentry_value_get_by_key(propagation_context, "trace"));
}

static void
init_scope(sentry_scope_t *scope)
{
    scope->release = NULL;
    scope->environment = NULL;
    scope->transaction = NULL;
    scope->fingerprint = sentry_value_new_null();
    scope->user = sentry_value_new_null();
    scope->tags = sentry_value_new_object();
    scope->extra = sentry_value_new_object();
    scope->attributes = sentry_value_new_object();
    scope->contexts = sentry_value_new_object();
    scope->propagation_context = sentry_value_new_object();
    scope->breadcrumbs = sentry__ringbuffer_new(SENTRY_BREADCRUMBS_MAX);
    scope->dynamic_sampling_context = sentry_value_new_object();
    scope->level = SENTRY_LEVEL_ERROR;
    scope->client_sdk = sentry_value_new_null();
    scope->attachments = NULL;
    scope->transaction_object = NULL;
    scope->span = NULL;
    scope->trace_managed = true;
    scope->observers = NULL;
    scope->num_observers = 0;
    scope->is_notifying = 0;
    scope->pending_flush = false;
    scope->one_shot = false;
}

static sentry_scope_t *
get_scope(void)
{
    if (g_scope_initialized) {
        return &g_scope;
    }

    memset(&g_scope, 0, sizeof(sentry_scope_t));
    sentry__rwlock_init(&g_scope.rwlock);
    init_scope(&g_scope);
    g_scope.user = sentry_value_new_object();
    sentry_value_set_by_key(g_scope.contexts, "os", sentry__get_os_context());
    g_scope.client_sdk = get_client_sdk();

    g_scope_initialized = true;

    return &g_scope;
}

static void
cleanup_scope(sentry_scope_t *scope)
{
    sentry_free(scope->release);
    sentry_free(scope->environment);
    sentry_free(scope->transaction);
    sentry_value_decref(scope->fingerprint);
    sentry_value_decref(scope->user);
    sentry_value_decref(scope->tags);
    sentry_value_decref(scope->extra);
    sentry_value_decref(scope->attributes);
    sentry_value_decref(scope->contexts);
    sentry_value_decref(scope->propagation_context);
    sentry__ringbuffer_free(scope->breadcrumbs);
    sentry_value_decref(scope->dynamic_sampling_context);
    sentry_value_decref(scope->client_sdk);
    sentry__attachments_free(scope->attachments);
    sentry__transaction_decref(scope->transaction_object);
    sentry__span_decref(scope->span);
    for (size_t i = 0; i < scope->num_observers; i++) {
        sentry_free(scope->observers[i]);
    }
    sentry_free(scope->observers);
    scope->observers = NULL;
    scope->num_observers = 0;
    scope->pending_flush = false;
}

void
sentry__scope_cleanup(void)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_lock);
    sentry__mutex_lock(&g_lock);
    if (g_scope_initialized) {
        g_scope_initialized = false;
        cleanup_scope(&g_scope);
        sentry__rwlock_free(&g_scope.rwlock);
    }
    sentry__mutex_unlock(&g_lock);
}

sentry_scope_t *
sentry__scope_lock(void)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_lock);
    sentry__mutex_lock(&g_lock);
    return get_scope();
}

static void
unlock_scope(bool flush)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_lock);

    if (g_scope.is_notifying > 0) {
        // defer the flush requested by a reentrant scope change
        g_scope.pending_flush = flush || g_scope.pending_flush;
        flush = false;
    } else {
        // consume any flush requested by a reentrant scope change
        flush = flush || g_scope.pending_flush;
        g_scope.pending_flush = false;
    }

    // we try to unlock the scope as soon as possible. The
    // backend will do its own `WITH_SCOPE` internally.
    sentry__mutex_unlock(&g_lock);
    if (flush) {
        SENTRY_WITH_OPTIONS (options) {
            if (options->backend && options->backend->flush_scope_func) {
                options->backend->flush_scope_func(options->backend, options);
            }
        }
    }
}

void
sentry__scope_unlock(void)
{
    unlock_scope(false);
}

void
sentry__scope_flush_unlock(void)
{
    unlock_scope(true);
}

sentry_scope_observer_t *
sentry__scope_observer_new(void)
{
    return SENTRY_MAKE(sentry_scope_observer_t);
}

bool
sentry__scope_add_observer(
    sentry_scope_t *scope, sentry_scope_observer_t *observer)
{
    if (!observer) {
        return false;
    }

    size_t new_count = scope->num_observers + 1;
    sentry_scope_observer_t **new_array
        = sentry__calloc(new_count, sizeof(sentry_scope_observer_t *));
    if (!new_array) {
        sentry_free(observer);
        return false;
    }
    if (scope->observers) {
        memcpy(new_array, scope->observers,
            scope->num_observers * sizeof(sentry_scope_observer_t *));
        sentry_free(scope->observers);
    }
    new_array[scope->num_observers] = observer;
    scope->observers = new_array;
    scope->num_observers = new_count;
    return true;
}

void
sentry__scope_remove_observer(
    sentry_scope_t *scope, sentry_scope_observer_t *observer)
{
    if (!observer || !scope->observers) {
        return;
    }

    for (size_t i = 0; i < scope->num_observers; i++) {
        if (scope->observers[i] != observer) {
            continue;
        }

        sentry_free(observer);
        if (scope->is_notifying) {
            // avoid shifting the array while SENTRY_SCOPE_NOTIFY is iterating
            scope->observers[i] = NULL;
            return;
        }
        for (size_t j = i + 1; j < scope->num_observers; j++) {
            scope->observers[j - 1] = scope->observers[j];
        }
        scope->num_observers--;
        if (scope->num_observers == 0) {
            sentry_free(scope->observers);
            scope->observers = NULL;
        }
        return;
    }
}

size_t
sentry__scope_begin_notify(sentry_scope_t *scope)
{
    scope->is_notifying++;
    return scope->num_observers;
}

void
sentry__scope_end_notify(sentry_scope_t *scope)
{
    if (--scope->is_notifying > 0) {
        return;
    }
    if (!scope->observers) {
        return;
    }

    // removals during notification leave tombstones to avoid shifting the array
    // while it is being iterated
    size_t j = 0;
    for (size_t i = 0; i < scope->num_observers; i++) {
        if (scope->observers[i]) {
            scope->observers[j++] = scope->observers[i];
        }
    }
    scope->num_observers = j;
    if (scope->num_observers == 0) {
        sentry_free(scope->observers);
        scope->observers = NULL;
    }
}

sentry_scope_t *
sentry_scope_new(void)
{
    sentry_scope_t *scope = SENTRY_MAKE(sentry_scope_t);
    if (!scope) {
        return NULL;
    }

    sentry__rwlock_init(&scope->rwlock);
    init_scope(scope);
    return scope;
}

void
sentry_scope_free(sentry_scope_t *scope)
{
    if (!scope) {
        return;
    }

    cleanup_scope(scope);
    sentry__rwlock_free(&scope->rwlock);
    sentry_free(scope);
}

bool
sentry__scope_is_one_shot(const sentry_scope_t *scope)
{
    bool one_shot = false;
    SCOPE_READ_LOCK(scope) { one_shot = scope->one_shot; }
    return one_shot;
}

void
sentry__scope_set_one_shot(sentry_scope_t *scope, bool one_shot)
{
    SCOPE_WRITE_LOCK(scope) { scope->one_shot = one_shot; }
}

void
sentry__scope_free_one_shot(sentry_scope_t *scope)
{
    if (sentry__scope_is_one_shot(scope)) {
        sentry_scope_free(scope);
    }
}

sentry_scope_t *
sentry_local_scope_new(void)
{
    sentry_scope_t *scope = sentry_scope_new();
    if (scope) {
        sentry__scope_set_one_shot(scope, true);
    }
    return scope;
}

void
sentry__scope_apply_options(sentry_scope_t *scope, sentry_options_t *options)
{
    if (options->sdk_name) {
        sentry_value_t sdk_name = sentry_value_new_string(options->sdk_name);
        sentry_value_set_by_key(scope->client_sdk, "name", sdk_name);
    }
    sentry_value_freeze(scope->client_sdk);
    generate_propagation_context(scope->propagation_context);
    sentry__scope_set_release_n(
        scope, options->release, sentry__guarded_strlen(options->release));
    sentry__scope_set_environment_n(scope, options->environment,
        sentry__guarded_strlen(options->environment));
    scope->attachments = options->attachments;
    options->attachments = NULL;

    sentry__ringbuffer_set_max_size(
        scope->breadcrumbs, options->max_breadcrumbs);

    sentry__scope_update_dsc(scope, options);
}

void
sentry_scope_clear(sentry_scope_t *scope)
{
    if (!scope) {
        return;
    }

    size_t end = sentry__scope_begin_notify(scope);
    for (size_t i = 0; i < end && i < scope->num_observers; i++) {
        sentry_scope_observer_t *observer = scope->observers[i];
        if (observer && observer->clear) {
            observer->clear(observer->data);
        }
    }
    sentry__scope_end_notify(scope);

    sentry_scope_observer_t **observers = scope->observers;
    size_t num_observers = scope->num_observers;
    size_t is_notifying = scope->is_notifying;
    bool pending_flush = scope->pending_flush;
    scope->observers = NULL;
    scope->num_observers = 0;

    // Keep the propagation and dynamic sampling contexts across clears so
    // telemetry captured afterwards continues on the same trace.
    bool trace_managed = sentry__scope_is_trace_managed(scope);
    sentry_value_t propagation_context = scope->propagation_context;
    sentry_value_t dynamic_sampling_context = scope->dynamic_sampling_context;
    sentry_value_incref(propagation_context);
    sentry_value_incref(dynamic_sampling_context);
    bool one_shot = sentry__scope_is_one_shot(scope);

    cleanup_scope(scope);
    init_scope(scope);

    sentry_value_decref(scope->propagation_context);
    sentry_value_decref(scope->dynamic_sampling_context);
    scope->propagation_context = propagation_context;
    scope->dynamic_sampling_context = dynamic_sampling_context;
    sentry__scope_set_trace_managed(scope, trace_managed);
    sentry__scope_set_one_shot(scope, one_shot);
    scope->observers = observers;
    scope->num_observers = num_observers;
    scope->is_notifying = is_notifying;
    scope->pending_flush = pending_flush;
}

sentry_scope_t *
sentry_scope_clone(const sentry_scope_t *scope)
{
    if (!scope) {
        return NULL;
    }

    sentry_scope_t *clone = SENTRY_MAKE(sentry_scope_t);
    if (!clone) {
        return NULL;
    }

    sentry__rwlock_init(&clone->rwlock);

    clone->release = sentry__string_clone(scope->release);
    clone->environment = sentry__string_clone(scope->environment);
    clone->transaction = sentry__string_clone(scope->transaction);
    sentry_value_t fingerprint = sentry__scope_ref_fingerprint(scope);
    clone->fingerprint = sentry__value_clone(fingerprint);
    sentry_value_decref(fingerprint);

    sentry_value_t user = sentry__scope_ref_user(scope);
    clone->user = sentry__value_clone(user);
    sentry_value_decref(user);
    clone->tags = sentry__value_clone(scope->tags);
    clone->extra = sentry__value_clone(scope->extra);
    clone->attributes = sentry__value_clone(scope->attributes);
    clone->contexts = sentry__value_clone(scope->contexts);
    clone->propagation_context
        = sentry__value_clone(scope->propagation_context);
    clone->breadcrumbs = sentry__ringbuffer_clone(scope->breadcrumbs);
    clone->dynamic_sampling_context
        = sentry__value_clone(scope->dynamic_sampling_context);
    if (sentry_value_is_frozen(scope->dynamic_sampling_context)) {
        sentry_value_freeze(clone->dynamic_sampling_context);
    }
    clone->level = sentry__scope_get_level(scope);
    clone->client_sdk = sentry__value_clone(scope->client_sdk);
    sentry__attachments_extend(&clone->attachments, scope->attachments);

    clone->transaction_object = scope->transaction_object;
    sentry__transaction_incref(clone->transaction_object);
    clone->span = scope->span;
    sentry__span_incref(clone->span);
    clone->trace_managed = sentry__scope_is_trace_managed(scope);

    return clone;
}

sentry_value_t
sentry__scope_get_propagation_context(const sentry_scope_t *scope)
{
    return scope->propagation_context;
}

sentry_value_t
sentry__scope_get_trace_context(const sentry_scope_t *scope)
{
    return sentry_value_get_by_key(scope->propagation_context, "trace");
}

void
sentry__scope_set_propagation_context(
    sentry_scope_t *scope, const char *key, sentry_value_t value)
{
    sentry_value_set_by_key(scope->propagation_context, key, value);
}

void
sentry__scope_regenerate_propagation_context(sentry_scope_t *scope)
{
    generate_propagation_context(scope->propagation_context);
}

bool
sentry__scope_is_trace_managed(const sentry_scope_t *scope)
{
    bool managed = false;
    SCOPE_READ_LOCK(scope) { managed = scope->trace_managed; }
    return managed;
}

void
sentry__scope_set_trace_managed(sentry_scope_t *scope, bool managed)
{
    SCOPE_WRITE_LOCK(scope) { scope->trace_managed = managed; }
}

sentry_value_t
sentry__scope_get_dsc(const sentry_scope_t *scope)
{
    return scope->dynamic_sampling_context;
}

void
sentry__scope_freeze_dsc(sentry_scope_t *scope, sentry_value_t incoming)
{
    sentry_value_decref(scope->dynamic_sampling_context);
    sentry_value_t dsc = sentry_value_new_object();
    sentry__value_merge_objects(dsc, incoming);
    sentry_value_freeze(dsc);
    scope->dynamic_sampling_context = dsc;
}

void
sentry__scope_update_dsc(sentry_scope_t *scope, const sentry_options_t *options)
{
    sentry_value_decref(scope->dynamic_sampling_context);
    sentry_value_t dsc = sentry_value_new_object();

    if (options->dsn) {
        sentry_value_set_by_key(dsc, "public_key",
            sentry_value_new_string(options->dsn->public_key));
    }
    const char *org_id = sentry__options_get_org_id(options);
    if (org_id) {
        sentry_value_set_by_key(dsc, "org_id", sentry_value_new_string(org_id));
    }
    sentry_value_set_by_key(dsc, "sample_rate",
        sentry_value_new_double(options->traces_sample_rate));
    if (options->traces_sampler) {
        sentry_value_set_by_key(
            dsc, "sample_rate", sentry_value_new_double(1.0));
    }
    sentry_value_t sample_rand = sentry_value_get_by_key(
        sentry_value_get_by_key(scope->propagation_context, "trace"),
        "sample_rand");
    sentry_value_set_by_key(dsc, "sample_rand", sample_rand);
    sentry_value_incref(sample_rand);
    sentry_value_set_by_key(
        dsc, "release", sentry_value_new_string(scope->release));
    sentry_value_set_by_key(
        dsc, "environment", sentry_value_new_string(scope->environment));

    scope->dynamic_sampling_context = dsc;
}

#if !defined(SENTRY_PLATFORM_NX)
static void
sentry__foreach_stacktrace(
    sentry_value_t event, void (*func)(sentry_value_t stacktrace))
{
    // We have stacktraces at the following locations:
    // * `exception[.values].X.stacktrace`:
    //   https://develop.sentry.dev/sdk/event-payloads/exception/
    // * `threads[.values].X.stacktrace`:
    //   https://develop.sentry.dev/sdk/event-payloads/threads/

    sentry_value_t exception = sentry_value_get_by_key(event, "exception");
    if (sentry_value_get_type(exception) == SENTRY_VALUE_TYPE_OBJECT) {
        exception = sentry_value_get_by_key(exception, "values");
    }
    if (sentry_value_get_type(exception) == SENTRY_VALUE_TYPE_LIST) {
        size_t len = sentry_value_get_length(exception);
        for (size_t i = 0; i < len; i++) {
            sentry_value_t stacktrace = sentry_value_get_by_key(
                sentry_value_get_by_index(exception, i), "stacktrace");
            if (!sentry_value_is_null(stacktrace)) {
                func(stacktrace);
            }
        }
    }

    sentry_value_t threads = sentry_value_get_by_key(event, "threads");
    if (sentry_value_get_type(threads) == SENTRY_VALUE_TYPE_OBJECT) {
        threads = sentry_value_get_by_key(threads, "values");
    }
    if (sentry_value_get_type(threads) == SENTRY_VALUE_TYPE_LIST) {
        size_t len = sentry_value_get_length(threads);
        for (size_t i = 0; i < len; i++) {
            sentry_value_t stacktrace = sentry_value_get_by_key(
                sentry_value_get_by_index(threads, i), "stacktrace");
            if (!sentry_value_is_null(stacktrace)) {
                func(stacktrace);
            }
        }
    }
}

static void
sentry__symbolize_frame(const sentry_frame_info_t *info, void *data)
{
    // See https://develop.sentry.dev/sdk/event-payloads/stacktrace/
    sentry_value_t frame = *(sentry_value_t *)data;

    if (info->symbol
        && sentry_value_is_null(sentry_value_get_by_key(frame, "function"))) {
        sentry_value_set_by_key(
            frame, "function", sentry_value_new_string(info->symbol));
    }

    if (info->object_name
        && sentry_value_is_null(sentry_value_get_by_key(frame, "package"))) {
        sentry_value_set_by_key(
            frame, "package", sentry_value_new_string(info->object_name));
    }

    if (info->symbol_addr
        && sentry_value_is_null(
            sentry_value_get_by_key(frame, "symbol_addr"))) {
        sentry_value_set_by_key(frame, "symbol_addr",
            sentry__value_new_addr((uint64_t)(size_t)info->symbol_addr));
    }

    if (info->load_addr
        && sentry_value_is_null(sentry_value_get_by_key(frame, "image_addr"))) {
        sentry_value_set_by_key(frame, "image_addr",
            sentry__value_new_addr((uint64_t)(size_t)info->load_addr));
    }
}

static void
sentry__symbolize_stacktrace(sentry_value_t stacktrace)
{
    sentry_value_t frames = sentry_value_get_by_key(stacktrace, "frames");
    if (sentry_value_get_type(frames) != SENTRY_VALUE_TYPE_LIST) {
        return;
    }

    size_t len = sentry_value_get_length(frames);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t frame = sentry_value_get_by_index(frames, i);

        sentry_value_t addr_value
            = sentry_value_get_by_key(frame, "instruction_addr");
        if (sentry_value_is_null(addr_value)) {
            continue;
        }

        // The addr is saved as a hex-number inside the value.
        size_t addr
            = (size_t)strtoll(sentry_value_as_string(addr_value), NULL, 0);
        if (!addr) {
            continue;
        }
        sentry__symbolize((void *)addr, sentry__symbolize_frame, &frame);
    }
}
#endif

static sentry_value_t
get_span_or_transaction(const sentry_scope_t *scope)
{
    if (scope->span) {
        return scope->span->inner;
    } else if (scope->transaction_object) {
        return scope->transaction_object->inner;
    } else {
        return sentry_value_new_null();
    }
}

#ifdef SENTRY_UNITTEST
sentry_value_t
sentry__scope_get_span_or_transaction(void)
{
    sentry_value_t result = sentry_value_new_null();
    SENTRY_WITH_SCOPE (scope) {
        result = get_span_or_transaction(scope);
    }
    return result;
}

bool
sentry__scope_has_observers(const sentry_scope_t *scope)
{
    return scope->num_observers > 0;
}
#endif

void
sentry__scope_apply_to_event(const sentry_scope_t *scope,
    const sentry_options_t *options, sentry_value_t event,
    sentry_scope_mode_t mode)
{
#define IS_NULL(Key) sentry_value_is_null(sentry_value_get_by_key(event, Key))
#define SET(Key, Value) sentry_value_set_by_key(event, Key, Value)
#define PLACE_STRING(Key, Source)                                              \
    do {                                                                       \
        if (IS_NULL(Key) && !sentry__string_empty(Source)) {                   \
            SET(Key, sentry_value_new_string(Source));                         \
        }                                                                      \
    } while (0)
#define PLACE_VALUE(Key, Source)                                               \
    do {                                                                       \
        if (IS_NULL(Key) && !sentry_value_is_null(Source)) {                   \
            sentry_value_incref(Source);                                       \
            SET(Key, Source);                                                  \
        }                                                                      \
    } while (0)
#define PLACE_CLONED_VALUE(Key, Source)                                        \
    do {                                                                       \
        if (IS_NULL(Key) && !sentry_value_is_null(Source)) {                   \
            SET(Key, sentry__value_clone(Source));                             \
        }                                                                      \
    } while (0)

    PLACE_STRING("platform", "native");

    PLACE_STRING("release", scope->release);
    PLACE_STRING("dist", options->dist);
    PLACE_STRING("environment", scope->environment);

    // is not transaction and has no level
    if (IS_NULL("type") && IS_NULL("level")) {
        SET("level", sentry__value_new_level(sentry__scope_get_level(scope)));
    }

    sentry_value_t user = sentry__scope_ref_user(scope);
    if (sentry_value_get_type(user) == SENTRY_VALUE_TYPE_OBJECT) {
        if (options->run && options->run->installation_id) {
            // ensure event has a user object
            if (IS_NULL("user")) {
                SET("user", sentry__value_clone(user));
            }
            // patch missing user ID with installation ID
            sentry_value_t event_user = sentry_value_get_by_key(event, "user");
            if (sentry_value_get_type(event_user) == SENTRY_VALUE_TYPE_OBJECT
                && sentry_value_is_null(
                    sentry_value_get_by_key(event_user, "id"))) {
                sentry_value_set_by_key(event_user, "id",
                    sentry_value_new_string(options->run->installation_id));
            }
        } else if (sentry_value_get_length(user) > 0) {
            PLACE_CLONED_VALUE("user", user);
        }
    }
    sentry_value_decref(user);

    sentry_value_t fingerprint = sentry__scope_ref_fingerprint(scope);
    PLACE_CLONED_VALUE("fingerprint", fingerprint);
    sentry_value_decref(fingerprint);

    PLACE_STRING("transaction", scope->transaction);
    PLACE_VALUE("sdk", scope->client_sdk);

    sentry_value_t event_tags = sentry_value_get_by_key(event, "tags");
    if (sentry_value_is_null(event_tags)) {
        if (!sentry_value_is_null(scope->tags)) {
            PLACE_CLONED_VALUE("tags", scope->tags);
        }
    } else {
        sentry__value_merge_objects(event_tags, scope->tags);
    }
    sentry_value_t event_extra = sentry_value_get_by_key(event, "extra");
    if (sentry_value_is_null(event_extra)) {
        if (!sentry_value_is_null(scope->extra)) {
            PLACE_CLONED_VALUE("extra", scope->extra);
        }
    } else {
        sentry__value_merge_objects(event_extra, scope->extra);
    }

    bool is_transaction = sentry__event_is_transaction(event);
    sentry_value_t contexts = sentry__value_clone(scope->contexts);
    if (is_transaction && !sentry_value_is_null(contexts)) {
        sentry_value_remove_by_key(contexts, "trace");
    }

    // prep contexts sourced from scope; data about transaction on scope needs
    // to be extracted and inserted
    sentry_value_t scoped_txn_or_span = sentry_value_new_null();
    sentry_value_t scope_trace = sentry_value_new_null();
    if (!is_transaction) {
        scoped_txn_or_span = get_span_or_transaction(scope);
        scope_trace = sentry__value_get_trace_context(scoped_txn_or_span);
    }
    if (!sentry_value_is_null(scope_trace)) {
        if (sentry_value_is_null(contexts)) {
            contexts = sentry_value_new_object();
        }
        sentry_value_t scoped_txn_or_span_data
            = sentry_value_get_by_key(scoped_txn_or_span, "data");
        if (!sentry_value_is_null(scoped_txn_or_span_data)) {
            sentry_value_incref(scoped_txn_or_span_data);
            sentry_value_set_by_key(
                scope_trace, "data", scoped_txn_or_span_data);
        }
        sentry_value_set_by_key(contexts, "trace", scope_trace);
    }

    // merge contexts sourced from scope into the event
    sentry_value_t event_contexts = sentry_value_get_by_key(event, "contexts");
    // merge propagation context only when no scoped span or event trace exists
    if (!is_transaction && sentry_value_is_null(scope_trace)
        && sentry_value_is_null(
            sentry_value_get_by_key(event_contexts, "trace"))) {
        sentry__value_merge_objects(contexts, scope->propagation_context);
    }
    if (sentry_value_is_null(event_contexts)) {
        PLACE_VALUE("contexts", contexts);
    } else {
        sentry__value_merge_objects(event_contexts, contexts);
    }
    sentry_value_decref(contexts);

    if (mode & SENTRY_SCOPE_BREADCRUMBS) {
        sentry_value_t event_breadcrumbs
            = sentry_value_get_by_key(event, "breadcrumbs");
        sentry_value_t scope_breadcrumbs
            = sentry__ringbuffer_to_list(scope->breadcrumbs);
        sentry_value_set_by_key(event, "breadcrumbs",
            sentry__value_merge_breadcrumbs(event_breadcrumbs,
                scope_breadcrumbs, options->max_breadcrumbs));
        sentry_value_decref(scope_breadcrumbs);
    }

#if !defined(SENTRY_PLATFORM_NX)
    if (mode & SENTRY_SCOPE_MODULES) {
        sentry_value_t modules = sentry_get_modules_list();
        if (!sentry_value_is_null(modules)) {
            sentry_value_t debug_meta = sentry_value_new_object();
            sentry_value_set_by_key(debug_meta, "images", modules);
            sentry_value_set_by_key(event, "debug_meta", debug_meta);
        }
    }

    if (mode & SENTRY_SCOPE_STACKTRACES) {
        sentry__foreach_stacktrace(event, sentry__symbolize_stacktrace);
    }
#endif

#undef PLACE_CLONED_VALUE
#undef PLACE_VALUE
#undef PLACE_STRING
#undef SET
#undef IS_NULL
}

void
sentry_scope_add_breadcrumb(sentry_scope_t *scope, sentry_value_t breadcrumb)
{
    if (sentry__ringbuffer_append(scope->breadcrumbs, breadcrumb) == 0) {
        SENTRY_SCOPE_NOTIFY(scope, add_breadcrumb, breadcrumb);
    }
}

const sentry_ringbuffer_t *
sentry__scope_get_breadcrumbs(const sentry_scope_t *scope)
{
    return scope->breadcrumbs;
}

sentry_value_t
sentry__scope_ref_user(const sentry_scope_t *scope)
{
    sentry_value_t user = sentry_value_new_null();
    SCOPE_READ_LOCK(scope) { user = sentry_value_incref(scope->user); }
    return user;
}

void
sentry_scope_set_user(sentry_scope_t *scope, sentry_value_t user)
{
    sentry_value_t old_user = sentry_value_new_null();
    sentry_value_t notify_user = sentry_value_new_null();
    SCOPE_WRITE_LOCK(scope)
    {
        old_user = scope->user;
        scope->user = user;
        notify_user = sentry_value_incref(user);
    }
    sentry_value_decref(old_user);
    SENTRY_SCOPE_NOTIFY(scope, set_user, notify_user);
    sentry_value_decref(notify_user);
}

sentry_value_t
sentry__scope_get_tags(const sentry_scope_t *scope)
{
    return scope->tags;
}

void
sentry_scope_set_tag(sentry_scope_t *scope, const char *key, const char *value)
{
    if (sentry_value_set_by_key(
            scope->tags, key, sentry_value_new_string(value))
        == 0) {
        SENTRY_SCOPE_NOTIFY(scope, set_tag, key, value);
    }
}

void
sentry_scope_set_tag_n(sentry_scope_t *scope, const char *key, size_t key_len,
    const char *value, size_t value_len)
{
    char *k = sentry__string_clone_n(key, key_len);
    sentry_value_t v = sentry_value_new_string_n(value, value_len);
    if (sentry__value_set_by_key_owned(scope->tags, k, key_len, v) == 0) {
        SENTRY_SCOPE_NOTIFY(scope, set_tag, k, sentry_value_as_string(v));
    }
}

void
sentry__scope_remove_tag(sentry_scope_t *scope, const char *key)
{
    if (sentry_value_remove_by_key(scope->tags, key) == 0) {
        SENTRY_SCOPE_NOTIFY(scope, remove_tag, key);
    }
}

void
sentry__scope_remove_tag_n(
    sentry_scope_t *scope, const char *key, size_t key_len)
{
    char *k = sentry__value_remove_and_take_key_n(scope->tags, key, key_len);
    if (k) {
        SENTRY_SCOPE_NOTIFY(scope, remove_tag, k);
    }
    sentry_free(k);
}

sentry_value_t
sentry__scope_get_extra(const sentry_scope_t *scope)
{
    return scope->extra;
}

void
sentry_scope_set_extra(
    sentry_scope_t *scope, const char *key, sentry_value_t value)
{
    if (sentry_value_set_by_key(scope->extra, key, value) == 0) {
        SENTRY_SCOPE_NOTIFY(scope, set_extra, key, value);
    }
}

void
sentry_scope_set_extra_n(sentry_scope_t *scope, const char *key, size_t key_len,
    sentry_value_t value)
{
    char *k = sentry__string_clone_n(key, key_len);
    if (sentry__value_set_by_key_owned(scope->extra, k, key_len, value) == 0) {
        SENTRY_SCOPE_NOTIFY(scope, set_extra, k, value);
    }
}

void
sentry__scope_remove_extra(sentry_scope_t *scope, const char *key)
{
    if (sentry_value_remove_by_key(scope->extra, key) == 0) {
        SENTRY_SCOPE_NOTIFY(scope, remove_extra, key);
    }
}

void
sentry__scope_remove_extra_n(
    sentry_scope_t *scope, const char *key, size_t key_len)
{
    char *k = sentry__value_remove_and_take_key_n(scope->extra, key, key_len);
    if (k) {
        SENTRY_SCOPE_NOTIFY(scope, remove_extra, k);
    }
    sentry_free(k);
}

void
sentry_scope_set_attribute(
    sentry_scope_t *scope, const char *key, sentry_value_t attribute)
{
    sentry_scope_set_attribute_n(
        scope, key, sentry__guarded_strlen(key), attribute);
}

void
sentry_scope_set_attribute_n(sentry_scope_t *scope, const char *key,
    size_t key_len, sentry_value_t attribute)
{
    if (sentry_value_is_null(sentry_value_get_by_key(attribute, "value"))
        || sentry_value_is_null(sentry_value_get_by_key(attribute, "type"))) {
        SENTRY_DEBUG("Cannot set attribute with missing 'value' or 'type'");
        sentry_value_decref(attribute);
        return;
    }
    sentry_value_set_by_key_n(scope->attributes, key, key_len, attribute);
}

sentry_value_t
sentry__scope_get_attributes(const sentry_scope_t *scope)
{
    return scope->attributes;
}

void
sentry_scope_remove_attribute(sentry_scope_t *scope, const char *key)
{
    sentry_value_remove_by_key(scope->attributes, key);
}

void
sentry_scope_remove_attribute_n(
    sentry_scope_t *scope, const char *key, size_t key_len)
{
    sentry_value_remove_by_key_n(scope->attributes, key, key_len);
}

sentry_value_t
sentry__scope_get_contexts(const sentry_scope_t *scope)
{
    return scope->contexts;
}

void
sentry_scope_set_context(
    sentry_scope_t *scope, const char *key, sentry_value_t value)
{
    if (sentry_value_set_by_key(scope->contexts, key, value) == 0) {
        SENTRY_SCOPE_NOTIFY(scope, set_context, key, value);
    }
}

void
sentry_scope_set_context_n(sentry_scope_t *scope, const char *key,
    size_t key_len, sentry_value_t value)
{
    char *k = sentry__string_clone_n(key, key_len);
    if (sentry__value_set_by_key_owned(scope->contexts, k, key_len, value)
        == 0) {
        SENTRY_SCOPE_NOTIFY(scope, set_context, k, value);
    }
}

void
sentry_scope_update_context(
    sentry_scope_t *scope, const char *key, sentry_value_t value)
{
    sentry_scope_update_context_n(
        scope, key, sentry__guarded_strlen(key), value);
}

void
sentry_scope_update_context_n(sentry_scope_t *scope, const char *key,
    size_t key_len, sentry_value_t value)
{
    sentry_value_t context
        = sentry_value_get_by_key_n(scope->contexts, key, key_len);
    char *k = sentry__string_clone_n(key, key_len);
    if (sentry_value_is_null(context)) {
        if (sentry__value_set_by_key_owned(scope->contexts, k, key_len, value)
            != 0) {
            return;
        }
    } else {
        sentry__value_merge_objects(value, context);
        if (sentry__value_set_by_key_owned(scope->contexts, k, key_len, value)
            != 0) {
            return;
        }
    }
    SENTRY_SCOPE_NOTIFY(scope, set_context, k, value);
}

void
sentry__scope_remove_context(sentry_scope_t *scope, const char *key)
{
    if (sentry_value_remove_by_key(scope->contexts, key) == 0) {
        SENTRY_SCOPE_NOTIFY(scope, remove_context, key);
    }
}

void
sentry__scope_remove_context_n(
    sentry_scope_t *scope, const char *key, size_t key_len)
{
    char *k
        = sentry__value_remove_and_take_key_n(scope->contexts, key, key_len);
    if (k) {
        SENTRY_SCOPE_NOTIFY(scope, remove_context, k);
    }
    sentry_free(k);
}

sentry_value_t
sentry__scope_ref_fingerprint(const sentry_scope_t *scope)
{
    sentry_value_t fingerprint = sentry_value_new_null();
    SCOPE_READ_LOCK(scope)
    {
        fingerprint = sentry_value_incref(scope->fingerprint);
    }
    return fingerprint;
}

void
sentry__scope_set_fingerprint_va(
    sentry_scope_t *scope, const char *fingerprint, va_list va)
{
    sentry_value_t fingerprint_value = sentry_value_new_list();
    for (; fingerprint; fingerprint = va_arg(va, const char *)) {
        sentry_value_append(
            fingerprint_value, sentry_value_new_string(fingerprint));
    }

    sentry_scope_set_fingerprints(scope, fingerprint_value);
}

void
sentry__scope_set_fingerprint_nva(sentry_scope_t *scope,
    const char *fingerprint, size_t fingerprint_len, va_list va)
{
    sentry_value_t fingerprint_value = sentry_value_new_list();
    while (fingerprint) {
        sentry_value_append(fingerprint_value,
            sentry_value_new_string_n(fingerprint, fingerprint_len));
        fingerprint = va_arg(va, const char *);
        if (fingerprint) {
            fingerprint_len = va_arg(va, size_t);
        }
    }

    sentry_scope_set_fingerprints(scope, fingerprint_value);
}

void
sentry_scope_set_fingerprint(
    sentry_scope_t *scope, const char *fingerprint, ...)
{
    va_list va;
    va_start(va, fingerprint);

    sentry__scope_set_fingerprint_va(scope, fingerprint, va);

    va_end(va);
}

void
sentry_scope_set_fingerprint_n(
    sentry_scope_t *scope, const char *fingerprint, size_t fingerprint_len, ...)
{
    va_list va;
    va_start(va, fingerprint_len);

    sentry__scope_set_fingerprint_nva(scope, fingerprint, fingerprint_len, va);

    va_end(va);
}

void
sentry_scope_set_fingerprints(
    sentry_scope_t *scope, sentry_value_t fingerprints)
{
    if (sentry_value_get_type(fingerprints) != SENTRY_VALUE_TYPE_LIST) {
        SENTRY_WARN("invalid fingerprints type, expected list");
        return;
    }

    sentry_value_t old_fingerprint = sentry_value_new_null();
    sentry_value_t notify_fingerprint = sentry_value_new_null();
    SCOPE_WRITE_LOCK(scope)
    {
        old_fingerprint = scope->fingerprint;
        scope->fingerprint = fingerprints;
        notify_fingerprint = sentry_value_incref(fingerprints);
    }
    sentry_value_decref(old_fingerprint);
    SENTRY_SCOPE_NOTIFY(scope, set_fingerprint, notify_fingerprint);
    sentry_value_decref(notify_fingerprint);
}

void
sentry__scope_remove_fingerprint(sentry_scope_t *scope)
{
    sentry_value_t fingerprint = sentry_value_new_null();
    sentry_value_t old_fingerprint = sentry_value_new_null();
    sentry_value_t notify_fingerprint = sentry_value_new_null();
    SCOPE_WRITE_LOCK(scope)
    {
        old_fingerprint = scope->fingerprint;
        scope->fingerprint = fingerprint;
        notify_fingerprint = sentry_value_incref(fingerprint);
    }
    sentry_value_decref(old_fingerprint);
    SENTRY_SCOPE_NOTIFY(scope, set_fingerprint, notify_fingerprint);
    sentry_value_decref(notify_fingerprint);
}

sentry_level_t
sentry__scope_get_level(const sentry_scope_t *scope)
{
    sentry_level_t level = SENTRY_LEVEL_ERROR;
    SCOPE_READ_LOCK(scope) { level = scope->level; }
    return level;
}

void
sentry_scope_set_level(sentry_scope_t *scope, sentry_level_t level)
{
    SCOPE_WRITE_LOCK(scope) { scope->level = level; }
    SENTRY_SCOPE_NOTIFY(scope, set_level, level);
}

sentry_attachment_t *
sentry__scope_get_attachments(const sentry_scope_t *scope)
{
    return scope->attachments;
}

sentry_attachment_t *
sentry__scope_add_attachment(
    sentry_scope_t *scope, sentry_attachment_t *attachment)
{
    if (!attachment) {
        return NULL;
    }

    sentry_attachment_t *added
        = sentry__attachments_add(&scope->attachments, attachment);
    if (added == attachment) {
        SENTRY_SCOPE_NOTIFY(scope, add_attachment, attachment);
    }
    return added;
}

bool
sentry__scope_remove_attachment(
    sentry_scope_t *scope, sentry_attachment_t *attachment)
{
    return sentry__attachments_remove(&scope->attachments, attachment);
}

sentry_attachment_t *
sentry__scope_take_attachments(sentry_scope_t *scope)
{
    sentry_attachment_t *attachments = scope->attachments;
    scope->attachments = NULL;
    return attachments;
}

sentry_transaction_t *
sentry__scope_get_transaction_object(const sentry_scope_t *scope)
{
    return scope->transaction_object;
}

static bool
value_has_span_id(sentry_value_t value, const char *span_id)
{
    const char *value_span_id
        = sentry_value_as_string(sentry_value_get_by_key(value, "span_id"));
    return sentry__string_eq(value_span_id, span_id);
}

void
sentry__scope_set_transaction_object(
    sentry_scope_t *scope, sentry_transaction_t *transaction)
{
    sentry__transaction_incref(transaction);
    sentry__span_decref(scope->span);
    scope->span = NULL;
    sentry__transaction_decref(scope->transaction_object);
    scope->transaction_object = transaction;
}

bool
sentry__scope_remove_transaction_object(
    sentry_scope_t *scope, sentry_transaction_t *transaction)
{
    if (!transaction || scope->transaction_object != transaction) {
        return false;
    }

    scope->transaction_object = NULL;
    sentry__transaction_decref(transaction);
    return true;
}

bool
sentry__scope_remove_transaction_value(
    sentry_scope_t *scope, sentry_value_t transaction)
{
    const char *span_id = sentry_value_as_string(
        sentry_value_get_by_key(transaction, "span_id"));
    if (!scope->transaction_object
        || !value_has_span_id(scope->transaction_object->inner, span_id)) {
        return false;
    }

    sentry_transaction_t *transaction_object = scope->transaction_object;
    scope->transaction_object = NULL;
    sentry__transaction_decref(transaction_object);
    return true;
}

bool
sentry__scope_restore_transaction_object(
    sentry_scope_t *scope, sentry_transaction_t *transaction)
{
    if (scope->transaction_object || !transaction) {
        return false;
    }

    scope->transaction_object = transaction;
    return true;
}

sentry_span_t *
sentry__scope_get_span(const sentry_scope_t *scope)
{
    return scope->span;
}

void
sentry__scope_set_span(sentry_scope_t *scope, sentry_span_t *span)
{
    sentry__span_incref(span);
    sentry__transaction_decref(scope->transaction_object);
    scope->transaction_object = NULL;
    sentry__span_decref(scope->span);
    scope->span = span;
}

bool
sentry__scope_remove_span(sentry_scope_t *scope, sentry_span_t *span)
{
    if (!span || scope->span != span) {
        return false;
    }

    scope->span = NULL;
    sentry__span_decref(span);
    return true;
}

bool
sentry__scope_remove_span_value(sentry_scope_t *scope, sentry_value_t span)
{
    const char *span_id
        = sentry_value_as_string(sentry_value_get_by_key(span, "span_id"));
    if (!scope->span || !value_has_span_id(scope->span->inner, span_id)) {
        return false;
    }

    sentry_span_t *scope_span = scope->span;
    scope->span = NULL;
    sentry__span_decref(scope_span);
    return true;
}

bool
sentry__scope_restore_span(sentry_scope_t *scope, sentry_span_t *span)
{
    if (scope->span || !span) {
        return false;
    }

    scope->span = span;
    return true;
}

const char *
sentry__scope_get_release(const sentry_scope_t *scope)
{
    return scope->release;
}

void
sentry__scope_set_release_n(
    sentry_scope_t *scope, const char *release, size_t release_len)
{
    sentry_free(scope->release);
    scope->release = sentry__string_clone_n(release, release_len);
    sentry_value_set_by_key(scope->dynamic_sampling_context, "release",
        sentry_value_new_string(scope->release));
    SENTRY_SCOPE_NOTIFY(scope, set_release, scope->release);
}

const char *
sentry__scope_get_environment(const sentry_scope_t *scope)
{
    return scope->environment;
}

void
sentry__scope_set_environment_n(
    sentry_scope_t *scope, const char *environment, size_t environment_len)
{
    sentry_free(scope->environment);
    scope->environment = sentry__string_clone_n(environment, environment_len);
    sentry_value_set_by_key(scope->dynamic_sampling_context, "environment",
        sentry_value_new_string(scope->environment));
    SENTRY_SCOPE_NOTIFY(scope, set_environment, scope->environment);
}

const char *
sentry__scope_get_transaction(const sentry_scope_t *scope)
{
    return scope->transaction;
}

void
sentry__scope_set_transaction_n(
    sentry_scope_t *scope, const char *transaction, size_t transaction_len)
{
    sentry_free(scope->transaction);
    scope->transaction = sentry__string_clone_n(transaction, transaction_len);

    if (scope->transaction_object) {
        sentry_transaction_set_name_n(
            scope->transaction_object, transaction, transaction_len);
    }
    SENTRY_SCOPE_NOTIFY(scope, set_transaction, scope->transaction);
}

sentry_attachment_t *
sentry_scope_attach_file(sentry_scope_t *scope, const char *path)
{
    return sentry_scope_attach_file_n(
        scope, path, sentry__guarded_strlen(path));
}

sentry_attachment_t *
sentry_scope_attach_file_n(
    sentry_scope_t *scope, const char *path, size_t path_len)
{
    return sentry__scope_add_attachment(scope,
        sentry__attachment_from_path(sentry__path_from_str_n(path, path_len)));
}

sentry_attachment_t *
sentry_scope_attach_bytes(sentry_scope_t *scope, const char *buf,
    size_t buf_len, const char *filename)
{
    return sentry_scope_attach_bytes_n(
        scope, buf, buf_len, filename, sentry__guarded_strlen(filename));
}

sentry_attachment_t *
sentry_scope_attach_bytes_n(sentry_scope_t *scope, const char *buf,
    size_t buf_len, const char *filename, size_t filename_len)
{
    return sentry__scope_add_attachment(scope,
        sentry__attachment_from_buffer(
            buf, buf_len, sentry__path_from_str_n(filename, filename_len)));
}

#ifdef SENTRY_PLATFORM_WINDOWS
sentry_attachment_t *
sentry_scope_attach_filew(sentry_scope_t *scope, const wchar_t *path)
{
    size_t path_len = path ? wcslen(path) : 0;
    return sentry_scope_attach_filew_n(scope, path, path_len);
}

sentry_attachment_t *
sentry_scope_attach_filew_n(
    sentry_scope_t *scope, const wchar_t *path, size_t path_len)
{
    return sentry__scope_add_attachment(scope,
        sentry__attachment_from_path(sentry__path_from_wstr_n(path, path_len)));
}

sentry_attachment_t *
sentry_scope_attach_bytesw(sentry_scope_t *scope, const char *buf,
    size_t buf_len, const wchar_t *filename)
{
    size_t filename_len = filename ? wcslen(filename) : 0;
    return sentry_scope_attach_bytesw_n(
        scope, buf, buf_len, filename, filename_len);
}

sentry_attachment_t *
sentry_scope_attach_bytesw_n(sentry_scope_t *scope, const char *buf,
    size_t buf_len, const wchar_t *filename, size_t filename_len)
{
    return sentry__scope_add_attachment(scope,
        sentry__attachment_from_buffer(
            buf, buf_len, sentry__path_from_wstr_n(filename, filename_len)));
}
#endif

void
sentry__scope_apply_to_telemetry(const sentry_scope_t *scope,
    sentry_value_t telemetry, sentry_value_t attributes)
{
    sentry__value_merge_objects_shallow(attributes, scope->attributes);

    // a span on the scope MUST take precedence over the propagation context
    sentry_value_t trace_id = sentry_value_get_by_key(
        sentry_value_get_by_key(scope->propagation_context, "trace"),
        "trace_id");

    sentry_value_t parent_span_id = sentry_value_new_object();
    if (scope->transaction_object) {
        sentry_value_t span_id = sentry_value_get_by_key(
            scope->transaction_object->inner, "span_id");
        sentry_value_incref(span_id);
        sentry_value_set_by_key(parent_span_id, "value", span_id);
        trace_id = sentry_value_get_by_key(
            scope->transaction_object->inner, "trace_id");
    } else if (scope->span) {
        sentry_value_t span_id
            = sentry_value_get_by_key(scope->span->inner, "span_id");
        sentry_value_incref(span_id);
        sentry_value_set_by_key(parent_span_id, "value", span_id);
        trace_id = sentry_value_get_by_key(scope->span->inner, "trace_id");
    }
    sentry_value_set_by_key(
        parent_span_id, "type", sentry_value_new_string("string"));
    if ((scope->transaction_object || scope->span)
        && sentry_value_is_null(sentry_value_get_by_key(
            attributes, "sentry.trace.parent_span_id"))) {
        sentry_value_set_by_key(
            attributes, "sentry.trace.parent_span_id", parent_span_id);
    } else {
        sentry_value_decref(parent_span_id);
    }
    if (!sentry_value_is_null(trace_id)
        && sentry_value_is_null(
            sentry_value_get_by_key(telemetry, "trace_id"))) {
        sentry_value_incref(trace_id);
        sentry_value_set_by_key(telemetry, "trace_id", trace_id);
    }

    sentry_value_t user = sentry__scope_ref_user(scope);
    if (!sentry_value_is_null(user)) {
        sentry_value_t user_id = sentry_value_get_by_key(user, "id");
        if (!sentry_value_is_null(user_id)) {
            sentry_value_incref(user_id);
            sentry__value_add_attribute(
                attributes, user_id, "string", "user.id");
        }

        sentry_value_t user_username
            = sentry_value_get_by_key(user, "username");
        if (!sentry_value_is_null(user_username)) {
            sentry_value_incref(user_username);
            sentry__value_add_attribute(
                attributes, user_username, "string", "user.name");
        }

        sentry_value_t user_email = sentry_value_get_by_key(user, "email");
        if (!sentry_value_is_null(user_email)) {
            sentry_value_incref(user_email);
            sentry__value_add_attribute(
                attributes, user_email, "string", "user.email");
        }
    }
    sentry_value_decref(user);

    sentry_value_t os_context = sentry_value_get_by_key(scope->contexts, "os");
    if (!sentry_value_is_null(os_context)) {
        sentry_value_t os_name = sentry_value_get_by_key(os_context, "name");
        sentry_value_t os_version
            = sentry_value_get_by_key(os_context, "version");
        if (!sentry_value_is_null(os_name)) {
            sentry_value_incref(os_name);
            sentry__value_add_attribute(
                attributes, os_name, "string", "os.name");
        }
        if (!sentry_value_is_null(os_version)) {
            sentry_value_incref(os_version);
            sentry__value_add_attribute(
                attributes, os_version, "string", "os.version");
        }
    }
    if (scope->environment) {
        sentry__value_add_attribute(attributes,
            sentry_value_new_string(scope->environment), "string",
            "sentry.environment");
    }
    if (scope->release) {
        sentry__value_add_attribute(attributes,
            sentry_value_new_string(scope->release), "string",
            "sentry.release");
    }
}
