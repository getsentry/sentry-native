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
#ifdef SENTRY__MUTEX_INIT_DYN
SENTRY__MUTEX_INIT_DYN(g_lock)
#else
static sentry_mutex_t g_lock = SENTRY__MUTEX_INIT;
#endif

struct sentry_scope_data_s {
    sentry_rwlock_t rwlock;

    sentry_value_t release;
    sentry_value_t environment;
    sentry_value_t transaction;
    sentry_value_t fingerprint;
    sentry_value_t user;
    sentry_value_t tags;
    sentry_value_t extra;
    sentry_value_t attributes;
    sentry_value_t contexts;
    sentry_value_t propagation_context;
    sentry_ringbuffer_t *breadcrumbs;
    sentry_value_t dynamic_sampling_context;
    sentry_level_t level;
    sentry_value_t client_sdk;
    sentry_attachment_t *attachments;

    // The span attached to this scope, if any.
    //
    // Conceptually, every transaction is a span, so it should be possible to
    // attach spans or transactions to a scope. But sentry_span_t and
    // sentry_transaction_t are unrelated types in the native SDK, so we need
    // two distinct pointers. At most one of them should ever be non-null.
    // Whenever possible, `transaction` should pull its value from the
    // `name` property nested in transaction_object or span.
    sentry_transaction_t *transaction_object;
    sentry_span_t *span;
    bool trace_managed;
};

static sentry_scope_t g_scope = { 0 };
static sentry_scope_data_t g_scope_data = { 0 };

#define DATA_READ_LOCK(Data)                                                   \
    for (const sentry_scope_data_t *_locked_data = (Data); _locked_data;       \
        sentry__rwlock_read_unlock((sentry_rwlock_t *)&_locked_data->rwlock),  \
                                   _locked_data = NULL)                        \
        for (bool _locked_once                                                 \
            = (sentry__rwlock_read_lock(                                       \
                   (sentry_rwlock_t *)&_locked_data->rwlock),                  \
                true);                                                         \
            _locked_once; _locked_once = false)

#define DATA_WRITE_LOCK(Data)                                                  \
    for (sentry_scope_data_t *_locked_data = (Data); _locked_data;             \
        sentry__rwlock_write_unlock(&_locked_data->rwlock),                    \
                             _locked_data = NULL)                              \
        for (bool _locked_once                                                 \
            = (sentry__rwlock_write_lock(&_locked_data->rwlock), true);        \
            _locked_once; _locked_once = false)

#define SENTRY_SCOPE_NOTIFY_OWNED(Scope, Callback, Value)                      \
    do {                                                                       \
        sentry_value_t _notify_value = (Value);                                \
        SENTRY_SCOPE_NOTIFY(Scope, Callback, _notify_value);                   \
        sentry_value_decref(_notify_value);                                    \
    } while (0)

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

static bool
data_update_context_by_key_n(sentry_scope_data_t *data, const char *key,
    size_t key_len, sentry_value_t value)
{
    sentry__rwlock_write_lock(&data->rwlock);
    sentry_value_t context
        = sentry_value_get_by_key_n(data->contexts, key, key_len);
    if (!sentry_value_is_null(context)) {
        sentry__value_merge_objects(value, context);
    }
    int rv = sentry_value_set_by_key_n(data->contexts, key, key_len, value);
    sentry__rwlock_write_unlock(&data->rwlock);
    return rv == 0;
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
init_data(sentry_scope_data_t *data)
{
    data->release = sentry_value_new_null();
    data->environment = sentry_value_new_null();
    data->transaction = sentry_value_new_null();
    data->fingerprint = sentry_value_new_null();
    data->user = sentry_value_new_null();
    data->tags = sentry_value_new_object();
    data->extra = sentry_value_new_object();
    data->attributes = sentry_value_new_object();
    data->contexts = sentry_value_new_object();
    data->propagation_context = sentry_value_new_object();
    data->breadcrumbs = sentry__ringbuffer_new(SENTRY_BREADCRUMBS_MAX);
    data->dynamic_sampling_context = sentry_value_new_object();
    data->level = SENTRY_LEVEL_ERROR;
    data->client_sdk = sentry_value_new_null();
    data->attachments = NULL;
    data->transaction_object = NULL;
    data->span = NULL;
    data->trace_managed = true;
}

static void
cleanup_data(sentry_scope_data_t *data)
{
    sentry_value_decref(data->release);
    sentry_value_decref(data->environment);
    sentry_value_decref(data->transaction);
    sentry_value_decref(data->fingerprint);
    sentry_value_decref(data->user);
    sentry_value_decref(data->tags);
    sentry_value_decref(data->extra);
    sentry_value_decref(data->attributes);
    sentry_value_decref(data->contexts);
    sentry_value_decref(data->propagation_context);
    sentry__ringbuffer_free(data->breadcrumbs);
    sentry_value_decref(data->dynamic_sampling_context);
    sentry_value_decref(data->client_sdk);
    sentry__attachments_free(data->attachments);
    sentry__transaction_decref(data->transaction_object);
    sentry__span_decref(data->span);
}

static sentry_scope_data_t *
new_data(void)
{
    sentry_scope_data_t *data = SENTRY_MAKE(sentry_scope_data_t);
    if (data) {
        sentry__rwlock_init(&data->rwlock);
        init_data(data);
    }
    return data;
}

static void
free_data(sentry_scope_data_t *data)
{
    if (!data) {
        return;
    }
    cleanup_data(data);
    sentry__rwlock_free(&data->rwlock);
    sentry_free(data);
}

static void
cleanup_global_data(sentry_scope_data_t *data)
{
    cleanup_data(data);
    sentry__rwlock_free(&data->rwlock);
}

static void
init_global_data(sentry_scope_data_t *data)
{
    sentry__value_replace(&data->user, sentry_value_new_object());
    sentry_value_set_by_key(data->contexts, "os", sentry__get_os_context());
    sentry__value_replace(&data->client_sdk, get_client_sdk());
}

static void
data_apply_options(sentry_scope_data_t *data, sentry_options_t *options)
{
    DATA_WRITE_LOCK(data)
    {
        if (options->sdk_name) {
            sentry_value_t sdk_name
                = sentry_value_new_string(options->sdk_name);
            sentry_value_set_by_key(data->client_sdk, "name", sdk_name);
        }
        sentry_value_freeze(data->client_sdk);
        generate_propagation_context(data->propagation_context);
        data->attachments = options->attachments;
        options->attachments = NULL;
        sentry__ringbuffer_set_max_size(
            data->breadcrumbs, options->max_breadcrumbs);
    }
}

static void
clear_data(sentry_scope_data_t *data)
{
    DATA_WRITE_LOCK(data)
    {
        bool trace_managed = data->trace_managed;
        sentry_value_t propagation_context
            = sentry_value_incref(data->propagation_context);
        sentry_value_t dynamic_sampling_context
            = sentry_value_incref(data->dynamic_sampling_context);

        cleanup_data(data);
        init_data(data);

        sentry_value_decref(data->propagation_context);
        sentry_value_decref(data->dynamic_sampling_context);
        data->propagation_context = propagation_context;
        data->dynamic_sampling_context = dynamic_sampling_context;
        data->trace_managed = trace_managed;
    }
}

static sentry_scope_data_t *
clone_data(const sentry_scope_data_t *source)
{
    sentry_scope_data_t *clone = SENTRY_MAKE(sentry_scope_data_t);
    if (!clone) {
        return NULL;
    }

    sentry__rwlock_init(&clone->rwlock);
    DATA_READ_LOCK(source)
    {
        clone->release = sentry__value_clone(source->release);
        clone->environment = sentry__value_clone(source->environment);
        clone->transaction = sentry__value_clone(source->transaction);
        clone->fingerprint = sentry__value_clone(source->fingerprint);
        clone->user = sentry__value_clone(source->user);
        clone->tags = sentry__value_clone(source->tags);
        clone->extra = sentry__value_clone(source->extra);
        clone->attributes = sentry__value_clone(source->attributes);
        clone->contexts = sentry__value_clone(source->contexts);
        clone->propagation_context
            = sentry__value_clone(source->propagation_context);
        clone->breadcrumbs = sentry__ringbuffer_clone(source->breadcrumbs);
        clone->dynamic_sampling_context
            = sentry__value_clone(source->dynamic_sampling_context);
        if (sentry_value_is_frozen(source->dynamic_sampling_context)) {
            sentry_value_freeze(clone->dynamic_sampling_context);
        }
        clone->level = source->level;
        clone->client_sdk = sentry__value_clone(source->client_sdk);
        sentry__attachments_extend(&clone->attachments, source->attachments);
        clone->transaction_object = source->transaction_object;
        sentry__transaction_incref(clone->transaction_object);
        clone->span = source->span;
        sentry__span_incref(clone->span);
        clone->trace_managed = source->trace_managed;
    }

    return clone;
}

static sentry_value_t
data_get_propagation_context(const sentry_scope_data_t *data)
{
    return data->propagation_context;
}

static sentry_value_t
data_ref_propagation_context(const sentry_scope_data_t *data)
{
    sentry_value_t propagation_context = sentry_value_new_null();
    DATA_READ_LOCK(data)
    {
        propagation_context = sentry_value_incref(data->propagation_context);
    }
    return propagation_context;
}

static sentry_value_t
data_get_trace_context(const sentry_scope_data_t *data)
{
    return sentry_value_get_by_key(data->propagation_context, "trace");
}

static void
data_set_propagation_context(
    sentry_scope_data_t *data, const char *key, sentry_value_t value)
{
    DATA_WRITE_LOCK(data)
    {
        sentry_value_set_by_key(data->propagation_context, key, value);
    }
}

static void
data_regenerate_propagation_context(sentry_scope_data_t *data)
{
    DATA_WRITE_LOCK(data)
    {
        generate_propagation_context(data->propagation_context);
    }
}

static bool
data_is_trace_managed(const sentry_scope_data_t *data)
{
    bool managed = false;
    DATA_READ_LOCK(data) { managed = data->trace_managed; }
    return managed;
}

static void
data_set_trace_managed(sentry_scope_data_t *data, bool managed)
{
    DATA_WRITE_LOCK(data) { data->trace_managed = managed; }
}

static sentry_value_t
data_get_dsc(const sentry_scope_data_t *data)
{
    return data->dynamic_sampling_context;
}

static void
data_freeze_dsc(sentry_scope_data_t *data, sentry_value_t incoming)
{
    sentry_value_t dsc = sentry_value_new_object();
    sentry__value_merge_objects(dsc, incoming);
    sentry_value_freeze(dsc);
    DATA_WRITE_LOCK(data)
    {
        sentry__value_replace(&data->dynamic_sampling_context, dsc);
    }
}

static void
data_update_dsc(sentry_scope_data_t *data, const sentry_options_t *options)
{
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

    DATA_WRITE_LOCK(data)
    {
        sentry_value_t sample_rand = sentry_value_get_by_key(
            sentry_value_get_by_key(data->propagation_context, "trace"),
            "sample_rand");
        sentry_value_set_by_key(
            dsc, "sample_rand", sentry_value_incref(sample_rand));
        sentry_value_set_by_key(
            dsc, "release", sentry_value_incref(data->release));
        sentry_value_set_by_key(
            dsc, "environment", sentry_value_incref(data->environment));
        sentry__value_replace(&data->dynamic_sampling_context, dsc);
    }
}

static sentry_value_t
data_ref_client_sdk(const sentry_scope_data_t *data)
{
    sentry_value_t client_sdk = sentry_value_new_null();
    DATA_READ_LOCK(data) { client_sdk = sentry_value_incref(data->client_sdk); }
    return client_sdk;
}

static void
data_apply_tags_and_extra(const sentry_scope_data_t *data, sentry_value_t event)
{
    DATA_READ_LOCK(data)
    {
        sentry_value_t event_tags = sentry_value_get_by_key(event, "tags");
        if (sentry_value_is_null(event_tags)) {
            if (!sentry_value_is_null(data->tags)) {
                sentry_value_set_by_key(
                    event, "tags", sentry__value_clone(data->tags));
            }
        } else {
            sentry__value_merge_objects(event_tags, data->tags);
        }

        sentry_value_t event_extra = sentry_value_get_by_key(event, "extra");
        if (sentry_value_is_null(event_extra)) {
            if (!sentry_value_is_null(data->extra)) {
                sentry_value_set_by_key(
                    event, "extra", sentry__value_clone(data->extra));
            }
        } else {
            sentry__value_merge_objects(event_extra, data->extra);
        }
    }
}

static sentry_value_t
data_clone_contexts(const sentry_scope_data_t *data)
{
    sentry_value_t contexts = sentry_value_new_null();
    DATA_READ_LOCK(data) { contexts = sentry__value_clone(data->contexts); }
    return contexts;
}

static sentry_value_t
data_ref_span_or_transaction(const sentry_scope_data_t *data)
{
    sentry_value_t value = sentry_value_new_null();
    DATA_READ_LOCK(data)
    {
        if (data->span) {
            value = sentry_value_incref(data->span->inner);
        } else if (data->transaction_object) {
            value = sentry_value_incref(data->transaction_object->inner);
        }
    }
    return value;
}

static sentry_value_t
data_breadcrumbs_to_list(const sentry_scope_data_t *data)
{
    sentry_value_t breadcrumbs = sentry_value_new_null();
    DATA_READ_LOCK(data)
    {
        breadcrumbs = sentry__ringbuffer_to_list(data->breadcrumbs);
    }
    return breadcrumbs;
}

static bool
data_add_breadcrumb(sentry_scope_data_t *data, sentry_value_t breadcrumb)
{
    bool added = false;
    DATA_WRITE_LOCK(data)
    {
        added = sentry__ringbuffer_append(data->breadcrumbs, breadcrumb) == 0;
    }
    return added;
}

static const sentry_ringbuffer_t *
data_get_breadcrumbs(const sentry_scope_data_t *data)
{
    return data->breadcrumbs;
}

static sentry_value_t
data_ref_user(const sentry_scope_data_t *data)
{
    sentry_value_t user = sentry_value_new_null();
    DATA_READ_LOCK(data) { user = sentry_value_incref(data->user); }
    return user;
}

static void
data_set_user(sentry_scope_data_t *data, sentry_value_t user)
{
    DATA_WRITE_LOCK(data)
    {
        sentry__value_replace(&data->user, sentry_value_incref(user));
    }
}

static sentry_value_t
data_get_tags(const sentry_scope_data_t *data)
{
    return data->tags;
}

static bool
data_set_tag(sentry_scope_data_t *data, const char *key, sentry_value_t value)
{
    bool did_set = false;
    DATA_WRITE_LOCK(data)
    {
        did_set = sentry_value_set_by_key(data->tags, key, value) == 0;
    }
    return did_set;
}

static bool
data_set_tag_n(sentry_scope_data_t *data, const char *key, size_t key_len,
    sentry_value_t value)
{
    bool did_set = false;
    DATA_WRITE_LOCK(data)
    {
        did_set
            = sentry_value_set_by_key_n(data->tags, key, key_len, value) == 0;
    }
    return did_set;
}

static bool
data_remove_tag(sentry_scope_data_t *data, const char *key)
{
    bool removed = false;
    DATA_WRITE_LOCK(data)
    {
        removed = sentry_value_remove_by_key(data->tags, key) == 0;
    }
    return removed;
}

static char *
data_remove_tag_n(sentry_scope_data_t *data, const char *key, size_t key_len)
{
    char *removed_key = NULL;
    DATA_WRITE_LOCK(data)
    {
        removed_key
            = sentry__value_remove_and_take_key_n(data->tags, key, key_len);
    }
    return removed_key;
}

static sentry_value_t
data_get_extra(const sentry_scope_data_t *data)
{
    return data->extra;
}

static bool
data_set_extra(sentry_scope_data_t *data, const char *key, sentry_value_t value)
{
    bool did_set = false;
    DATA_WRITE_LOCK(data)
    {
        did_set = sentry_value_set_by_key(data->extra, key, value) == 0;
    }
    return did_set;
}

static bool
data_set_extra_n(sentry_scope_data_t *data, const char *key, size_t key_len,
    sentry_value_t value)
{
    bool did_set = false;
    DATA_WRITE_LOCK(data)
    {
        did_set
            = sentry_value_set_by_key_n(data->extra, key, key_len, value) == 0;
    }
    return did_set;
}

static bool
data_remove_extra(sentry_scope_data_t *data, const char *key)
{
    bool removed = false;
    DATA_WRITE_LOCK(data)
    {
        removed = sentry_value_remove_by_key(data->extra, key) == 0;
    }
    return removed;
}

static char *
data_remove_extra_n(sentry_scope_data_t *data, const char *key, size_t key_len)
{
    char *removed_key = NULL;
    DATA_WRITE_LOCK(data)
    {
        removed_key
            = sentry__value_remove_and_take_key_n(data->extra, key, key_len);
    }
    return removed_key;
}

static sentry_value_t
data_get_attributes(const sentry_scope_data_t *data)
{
    return data->attributes;
}

static bool
data_set_attribute_n(sentry_scope_data_t *data, const char *key, size_t key_len,
    sentry_value_t attribute)
{
    bool did_set = false;
    DATA_WRITE_LOCK(data)
    {
        did_set = sentry_value_set_by_key_n(
                      data->attributes, key, key_len, attribute)
            == 0;
    }
    return did_set;
}

static void
data_remove_attribute(sentry_scope_data_t *data, const char *key)
{
    DATA_WRITE_LOCK(data) { sentry_value_remove_by_key(data->attributes, key); }
}

static void
data_remove_attribute_n(
    sentry_scope_data_t *data, const char *key, size_t key_len)
{
    DATA_WRITE_LOCK(data)
    {
        sentry_value_remove_by_key_n(data->attributes, key, key_len);
    }
}

static sentry_value_t
data_get_contexts(const sentry_scope_data_t *data)
{
    return data->contexts;
}

static bool
data_set_context(
    sentry_scope_data_t *data, const char *key, sentry_value_t value)
{
    bool did_set = false;
    DATA_WRITE_LOCK(data)
    {
        did_set = sentry_value_set_by_key(data->contexts, key, value) == 0;
    }
    return did_set;
}

static bool
data_set_context_n(sentry_scope_data_t *data, const char *key, size_t key_len,
    sentry_value_t value)
{
    bool did_set = false;
    DATA_WRITE_LOCK(data)
    {
        did_set = sentry_value_set_by_key_n(data->contexts, key, key_len, value)
            == 0;
    }
    return did_set;
}

static bool
data_update_context_n(sentry_scope_data_t *data, const char *key,
    size_t key_len, sentry_value_t value)
{
    return data_update_context_by_key_n(data, key, key_len, value);
}

static bool
data_remove_context(sentry_scope_data_t *data, const char *key)
{
    bool removed = false;
    DATA_WRITE_LOCK(data)
    {
        removed = sentry_value_remove_by_key(data->contexts, key) == 0;
    }
    return removed;
}

static char *
data_remove_context_n(
    sentry_scope_data_t *data, const char *key, size_t key_len)
{
    char *removed_key = NULL;
    DATA_WRITE_LOCK(data)
    {
        removed_key
            = sentry__value_remove_and_take_key_n(data->contexts, key, key_len);
    }
    return removed_key;
}

static sentry_value_t
data_ref_fingerprint(const sentry_scope_data_t *data)
{
    sentry_value_t fingerprint = sentry_value_new_null();
    DATA_READ_LOCK(data)
    {
        fingerprint = sentry_value_incref(data->fingerprint);
    }
    return fingerprint;
}

static void
data_set_fingerprint(sentry_scope_data_t *data, sentry_value_t fingerprint)
{
    DATA_WRITE_LOCK(data)
    {
        sentry__value_replace(
            &data->fingerprint, sentry_value_incref(fingerprint));
    }
}

static sentry_level_t
data_get_level(const sentry_scope_data_t *data)
{
    sentry_level_t level = SENTRY_LEVEL_ERROR;
    DATA_READ_LOCK(data) { level = data->level; }
    return level;
}

static void
data_set_level(sentry_scope_data_t *data, sentry_level_t level)
{
    DATA_WRITE_LOCK(data) { data->level = level; }
}

static sentry_attachment_t *
data_get_attachments(const sentry_scope_data_t *data)
{
    return data->attachments;
}

static sentry_attachment_t *
data_add_attachment(sentry_scope_data_t *data, sentry_attachment_t *attachment)
{
    sentry_attachment_t *added = NULL;
    DATA_WRITE_LOCK(data)
    {
        added = sentry__attachments_add(&data->attachments, attachment);
    }
    return added;
}

static bool
data_remove_attachment(
    sentry_scope_data_t *data, sentry_attachment_t *attachment)
{
    bool removed = false;
    DATA_WRITE_LOCK(data)
    {
        removed = sentry__attachments_remove(&data->attachments, attachment);
    }
    return removed;
}

static sentry_attachment_t *
data_take_attachments(sentry_scope_data_t *data)
{
    sentry_attachment_t *attachments = NULL;
    DATA_WRITE_LOCK(data)
    {
        attachments = data->attachments;
        data->attachments = NULL;
    }
    return attachments;
}

static sentry_transaction_t *
data_get_transaction_object(const sentry_scope_data_t *data)
{
    sentry_transaction_t *transaction = NULL;
    DATA_READ_LOCK(data) { transaction = data->transaction_object; }
    return transaction;
}

static bool
value_has_span_id(sentry_value_t value, const char *span_id)
{
    const char *value_span_id
        = sentry_value_as_string(sentry_value_get_by_key(value, "span_id"));
    return sentry__string_eq(value_span_id, span_id);
}

static void
data_set_transaction_object(
    sentry_scope_data_t *data, sentry_transaction_t *transaction)
{
    sentry__transaction_incref(transaction);
    DATA_WRITE_LOCK(data)
    {
        sentry__span_decref(data->span);
        data->span = NULL;
        sentry__transaction_decref(data->transaction_object);
        data->transaction_object = transaction;
    }
}

static bool
data_remove_transaction_object(
    sentry_scope_data_t *data, sentry_transaction_t *transaction)
{
    bool removed = false;
    DATA_WRITE_LOCK(data)
    {
        if (transaction && data->transaction_object == transaction) {
            data->transaction_object = NULL;
            removed = true;
        }
    }
    if (removed) {
        sentry__transaction_decref(transaction);
    }
    return removed;
}

static bool
data_remove_transaction_value(
    sentry_scope_data_t *data, sentry_value_t transaction)
{
    const char *span_id = sentry_value_as_string(
        sentry_value_get_by_key(transaction, "span_id"));
    sentry_transaction_t *transaction_object = NULL;
    DATA_WRITE_LOCK(data)
    {
        if (data->transaction_object
            && value_has_span_id(data->transaction_object->inner, span_id)) {
            transaction_object = data->transaction_object;
            data->transaction_object = NULL;
        }
    }
    sentry__transaction_decref(transaction_object);
    return transaction_object != NULL;
}

static bool
data_restore_transaction_object(
    sentry_scope_data_t *data, sentry_transaction_t *transaction)
{
    bool restored = false;
    DATA_WRITE_LOCK(data)
    {
        if (!data->transaction_object && transaction) {
            data->transaction_object = transaction;
            restored = true;
        }
    }
    return restored;
}

static sentry_span_t *
data_get_span(const sentry_scope_data_t *data)
{
    sentry_span_t *span = NULL;
    DATA_READ_LOCK(data) { span = data->span; }
    return span;
}

static void
data_set_span(sentry_scope_data_t *data, sentry_span_t *span)
{
    sentry__span_incref(span);
    DATA_WRITE_LOCK(data)
    {
        sentry__transaction_decref(data->transaction_object);
        data->transaction_object = NULL;
        sentry__span_decref(data->span);
        data->span = span;
    }
}

static bool
data_remove_span(sentry_scope_data_t *data, sentry_span_t *span)
{
    bool removed = false;
    DATA_WRITE_LOCK(data)
    {
        if (span && data->span == span) {
            data->span = NULL;
            removed = true;
        }
    }
    if (removed) {
        sentry__span_decref(span);
    }
    return removed;
}

static bool
data_remove_span_value(sentry_scope_data_t *data, sentry_value_t span)
{
    const char *span_id
        = sentry_value_as_string(sentry_value_get_by_key(span, "span_id"));
    sentry_span_t *scope_span = NULL;
    DATA_WRITE_LOCK(data)
    {
        if (data->span && value_has_span_id(data->span->inner, span_id)) {
            scope_span = data->span;
            data->span = NULL;
        }
    }
    sentry__span_decref(scope_span);
    return scope_span != NULL;
}

static bool
data_restore_span(sentry_scope_data_t *data, sentry_span_t *span)
{
    bool restored = false;
    DATA_WRITE_LOCK(data)
    {
        if (!data->span && span) {
            data->span = span;
            restored = true;
        }
    }
    return restored;
}

static sentry_value_t
data_ref_release(const sentry_scope_data_t *data)
{
    sentry_value_t release = sentry_value_new_null();
    DATA_READ_LOCK(data) { release = sentry_value_incref(data->release); }
    return release;
}

static void
data_set_release(sentry_scope_data_t *data, sentry_value_t value)
{
    DATA_WRITE_LOCK(data)
    {
        sentry__value_replace(&data->release, sentry_value_incref(value));
        sentry_value_set_by_key(data->dynamic_sampling_context, "release",
            sentry_value_incref(value));
    }
}

static sentry_value_t
data_ref_environment(const sentry_scope_data_t *data)
{
    sentry_value_t environment = sentry_value_new_null();
    DATA_READ_LOCK(data)
    {
        environment = sentry_value_incref(data->environment);
    }
    return environment;
}

static void
data_set_environment(sentry_scope_data_t *data, sentry_value_t value)
{
    DATA_WRITE_LOCK(data)
    {
        sentry__value_replace(&data->environment, sentry_value_incref(value));
        sentry_value_set_by_key(data->dynamic_sampling_context, "environment",
            sentry_value_incref(value));
    }
}

static sentry_value_t
data_ref_transaction(const sentry_scope_data_t *data)
{
    sentry_value_t transaction = sentry_value_new_null();
    DATA_READ_LOCK(data)
    {
        transaction = sentry_value_incref(data->transaction);
    }
    return transaction;
}

static void
data_set_transaction(sentry_scope_data_t *data, sentry_value_t value,
    const char *transaction, size_t transaction_len)
{
    DATA_WRITE_LOCK(data)
    {
        sentry__value_replace(&data->transaction, sentry_value_incref(value));
        if (data->transaction_object) {
            sentry_transaction_set_name_n(
                data->transaction_object, transaction, transaction_len);
        }
    }
}

static void
data_apply_to_telemetry(const sentry_scope_data_t *data,
    sentry_value_t telemetry, sentry_value_t attributes)
{
    sentry_value_t os_name = sentry_value_new_null();
    sentry_value_t os_version = sentry_value_new_null();

    DATA_READ_LOCK(data)
    {
        sentry__value_merge_objects_shallow(attributes, data->attributes);

        sentry_value_t trace_id = sentry_value_get_by_key(
            sentry_value_get_by_key(data->propagation_context, "trace"),
            "trace_id");

        sentry_value_t parent_span_id = sentry_value_new_object();
        bool has_parent_span = false;
        if (data->transaction_object) {
            sentry_value_t span_id = sentry_value_get_by_key(
                data->transaction_object->inner, "span_id");
            sentry_value_set_by_key(
                parent_span_id, "value", sentry_value_incref(span_id));
            trace_id = sentry_value_get_by_key(
                data->transaction_object->inner, "trace_id");
            has_parent_span = true;
        } else if (data->span) {
            sentry_value_t span_id
                = sentry_value_get_by_key(data->span->inner, "span_id");
            sentry_value_set_by_key(
                parent_span_id, "value", sentry_value_incref(span_id));
            trace_id = sentry_value_get_by_key(data->span->inner, "trace_id");
            has_parent_span = true;
        }
        sentry_value_set_by_key(
            parent_span_id, "type", sentry_value_new_string("string"));
        if (has_parent_span
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
            sentry_value_set_by_key(
                telemetry, "trace_id", sentry_value_incref(trace_id));
        }

        sentry_value_t os_context
            = sentry_value_get_by_key(data->contexts, "os");
        if (!sentry_value_is_null(os_context)) {
            os_name = sentry_value_incref(
                sentry_value_get_by_key(os_context, "name"));
            os_version = sentry_value_incref(
                sentry_value_get_by_key(os_context, "version"));
        }
    }

    if (!sentry_value_is_null(os_name)) {
        sentry__value_add_attribute(attributes, os_name, "string", "os.name");
    } else {
        sentry_value_decref(os_name);
    }
    if (!sentry_value_is_null(os_version)) {
        sentry__value_add_attribute(
            attributes, os_version, "string", "os.version");
    } else {
        sentry_value_decref(os_version);
    }
}

static bool
init_scope(sentry_scope_t *scope, sentry_scope_data_t *data)
{
    scope->data = data ? data : new_data();
    if (!scope->data) {
        return false;
    }
    scope->observers = NULL;
    scope->num_observers = 0;
    scope->is_notifying = 0;
    scope->pending_flush = false;
    scope->one_shot = false;
    return true;
}

static sentry_scope_t *
get_scope(void)
{
    if (g_scope_initialized) {
        return &g_scope;
    }

    memset(&g_scope, 0, sizeof(sentry_scope_t));
    memset(&g_scope_data, 0, sizeof(sentry_scope_data_t));
    sentry__rwlock_init(&g_scope_data.rwlock);
    init_data(&g_scope_data);
    if (!init_scope(&g_scope, &g_scope_data)) {
        return &g_scope;
    }
    init_global_data(g_scope.data);

    g_scope_initialized = true;

    return &g_scope;
}

static void
cleanup_observers(sentry_scope_t *scope)
{
    for (size_t i = 0; i < scope->num_observers; i++) {
        sentry_free(scope->observers[i]);
    }
    sentry_free(scope->observers);
    scope->observers = NULL;
    scope->num_observers = 0;
    scope->pending_flush = false;
}

static void
cleanup_scope(sentry_scope_t *scope)
{
    free_data(scope->data);
    scope->data = NULL;
    cleanup_observers(scope);
}

void
sentry__scope_cleanup(void)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_lock);
    sentry__mutex_lock(&g_lock);
    if (g_scope_initialized) {
        g_scope_initialized = false;
        cleanup_global_data(g_scope.data);
        g_scope.data = NULL;
        cleanup_observers(&g_scope);
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

    if (!init_scope(scope, NULL)) {
        sentry_free(scope);
        return NULL;
    }
    return scope;
}

void
sentry_scope_free(sentry_scope_t *scope)
{
    if (!scope) {
        return;
    }

    cleanup_scope(scope);
    sentry_free(scope);
}

bool
sentry__scope_is_one_shot(const sentry_scope_t *scope)
{
    return scope && scope->one_shot;
}

void
sentry__scope_set_one_shot(sentry_scope_t *scope, bool one_shot)
{
    if (!scope) {
        return;
    }
    scope->one_shot = one_shot;
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
    data_apply_options(scope->data, options);
    sentry__scope_set_release_n(
        scope, options->release, sentry__guarded_strlen(options->release));
    sentry__scope_set_environment_n(scope, options->environment,
        sentry__guarded_strlen(options->environment));
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

    clear_data(scope->data);
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

    clone->data = clone_data(scope->data);
    if (!clone->data) {
        sentry_free(clone);
        return NULL;
    }

    return clone;
}

sentry_value_t
sentry__scope_get_propagation_context(const sentry_scope_t *scope)
{
    return data_get_propagation_context(scope->data);
}

sentry_value_t
sentry__scope_get_trace_context(const sentry_scope_t *scope)
{
    return data_get_trace_context(scope->data);
}

void
sentry__scope_set_propagation_context(
    sentry_scope_t *scope, const char *key, sentry_value_t value)
{
    data_set_propagation_context(scope->data, key, value);
}

void
sentry__scope_regenerate_propagation_context(sentry_scope_t *scope)
{
    data_regenerate_propagation_context(scope->data);
}

bool
sentry__scope_is_trace_managed(const sentry_scope_t *scope)
{
    return data_is_trace_managed(scope->data);
}

void
sentry__scope_set_trace_managed(sentry_scope_t *scope, bool managed)
{
    data_set_trace_managed(scope->data, managed);
}

sentry_value_t
sentry__scope_get_dsc(const sentry_scope_t *scope)
{
    return data_get_dsc(scope->data);
}

void
sentry__scope_freeze_dsc(sentry_scope_t *scope, sentry_value_t incoming)
{
    data_freeze_dsc(scope->data, incoming);
}

void
sentry__scope_update_dsc(sentry_scope_t *scope, const sentry_options_t *options)
{
    data_update_dsc(scope->data, options);
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

#ifdef SENTRY_UNITTEST
static sentry_value_t
data_get_span_or_transaction(const sentry_scope_data_t *data)
{
    sentry_value_t value = sentry_value_new_null();
    DATA_READ_LOCK(data)
    {
        if (data->span) {
            value = data->span->inner;
        } else if (data->transaction_object) {
            value = data->transaction_object->inner;
        }
    }
    return value;
}

sentry_value_t
sentry__scope_get_span_or_transaction(void)
{
    sentry_value_t result = sentry_value_new_null();
    SENTRY_WITH_SCOPE (scope) {
        result = data_get_span_or_transaction(scope->data);
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
#define PLACE_STRING_VALUE(Key, Source)                                        \
    do {                                                                       \
        if (IS_NULL(Key) && sentry_value_get_length(Source) > 0) {             \
            SET(Key, sentry_value_incref(Source));                             \
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

    sentry_value_t release = sentry__scope_ref_release(scope);
    PLACE_STRING_VALUE("release", release);
    sentry_value_decref(release);

    PLACE_STRING("dist", options->dist);

    sentry_value_t environment = sentry__scope_ref_environment(scope);
    PLACE_STRING_VALUE("environment", environment);
    sentry_value_decref(environment);

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

    sentry_value_t transaction = sentry__scope_ref_transaction(scope);
    PLACE_STRING_VALUE("transaction", transaction);
    sentry_value_decref(transaction);

    sentry_value_t client_sdk = data_ref_client_sdk(scope->data);
    PLACE_VALUE("sdk", client_sdk);
    sentry_value_decref(client_sdk);

    data_apply_tags_and_extra(scope->data, event);

    bool is_transaction = sentry__event_is_transaction(event);
    sentry_value_t contexts = data_clone_contexts(scope->data);
    if (is_transaction && !sentry_value_is_null(contexts)) {
        sentry_value_remove_by_key(contexts, "trace");
    }

    // prep contexts sourced from scope; data about transaction on scope needs
    // to be extracted and inserted
    sentry_value_t scoped_txn_or_span = sentry_value_new_null();
    sentry_value_t scope_trace = sentry_value_new_null();
    if (!is_transaction) {
        scoped_txn_or_span = data_ref_span_or_transaction(scope->data);
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
    sentry_value_decref(scoped_txn_or_span);

    // merge contexts sourced from scope into the event
    sentry_value_t event_contexts = sentry_value_get_by_key(event, "contexts");
    // merge propagation context only when no scoped span or event trace exists
    if (!is_transaction && sentry_value_is_null(scope_trace)
        && sentry_value_is_null(
            sentry_value_get_by_key(event_contexts, "trace"))) {
        sentry_value_t propagation_context
            = data_ref_propagation_context(scope->data);
        sentry__value_merge_objects(contexts, propagation_context);
        sentry_value_decref(propagation_context);
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
            = data_breadcrumbs_to_list(scope->data);
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
#undef PLACE_STRING_VALUE
#undef PLACE_STRING
#undef SET
#undef IS_NULL
}

void
sentry_scope_add_breadcrumb(sentry_scope_t *scope, sentry_value_t breadcrumb)
{
    if (data_add_breadcrumb(scope->data, breadcrumb)) {
        SENTRY_SCOPE_NOTIFY(scope, add_breadcrumb, breadcrumb);
    }
}

const sentry_ringbuffer_t *
sentry__scope_get_breadcrumbs(const sentry_scope_t *scope)
{
    return data_get_breadcrumbs(scope->data);
}

sentry_value_t
sentry__scope_ref_user(const sentry_scope_t *scope)
{
    return data_ref_user(scope->data);
}

void
sentry_scope_set_user(sentry_scope_t *scope, sentry_value_t user)
{
    data_set_user(scope->data, user);
    SENTRY_SCOPE_NOTIFY_OWNED(scope, set_user, user);
}

sentry_value_t
sentry__scope_get_tags(const sentry_scope_t *scope)
{
    return data_get_tags(scope->data);
}

void
sentry_scope_set_tag(sentry_scope_t *scope, const char *key, const char *value)
{
    if (data_set_tag(scope->data, key, sentry_value_new_string(value))) {
        SENTRY_SCOPE_NOTIFY(scope, set_tag, key, value);
    }
}

void
sentry_scope_set_tag_n(sentry_scope_t *scope, const char *key, size_t key_len,
    const char *value, size_t value_len)
{
    sentry_value_t tag_value = sentry_value_new_string_n(value, value_len);
    char *notify_key = sentry__string_clone_n(key, key_len);
    if (!notify_key) {
        sentry_value_decref(tag_value);
        return;
    }

    if (data_set_tag_n(
            scope->data, key, key_len, sentry_value_incref(tag_value))) {
        SENTRY_SCOPE_NOTIFY(
            scope, set_tag, notify_key, sentry_value_as_string(tag_value));
    }
    sentry_free(notify_key);
    sentry_value_decref(tag_value);
}

void
sentry__scope_remove_tag(sentry_scope_t *scope, const char *key)
{
    if (data_remove_tag(scope->data, key)) {
        SENTRY_SCOPE_NOTIFY(scope, remove_tag, key);
    }
}

void
sentry__scope_remove_tag_n(
    sentry_scope_t *scope, const char *key, size_t key_len)
{
    char *k = data_remove_tag_n(scope->data, key, key_len);
    if (k) {
        SENTRY_SCOPE_NOTIFY(scope, remove_tag, k);
    }
    sentry_free(k);
}

sentry_value_t
sentry__scope_get_extra(const sentry_scope_t *scope)
{
    return data_get_extra(scope->data);
}

void
sentry_scope_set_extra(
    sentry_scope_t *scope, const char *key, sentry_value_t value)
{
    if (data_set_extra(scope->data, key, sentry_value_incref(value))) {
        SENTRY_SCOPE_NOTIFY(scope, set_extra, key, value);
    }
    sentry_value_decref(value);
}

void
sentry_scope_set_extra_n(sentry_scope_t *scope, const char *key, size_t key_len,
    sentry_value_t value)
{
    char *notify_key = sentry__string_clone_n(key, key_len);
    if (!notify_key) {
        sentry_value_decref(value);
        return;
    }

    if (data_set_extra_n(
            scope->data, key, key_len, sentry_value_incref(value))) {
        SENTRY_SCOPE_NOTIFY(scope, set_extra, notify_key, value);
    }
    sentry_free(notify_key);
    sentry_value_decref(value);
}

void
sentry__scope_remove_extra(sentry_scope_t *scope, const char *key)
{
    if (data_remove_extra(scope->data, key)) {
        SENTRY_SCOPE_NOTIFY(scope, remove_extra, key);
    }
}

void
sentry__scope_remove_extra_n(
    sentry_scope_t *scope, const char *key, size_t key_len)
{
    char *k = data_remove_extra_n(scope->data, key, key_len);
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
    data_set_attribute_n(scope->data, key, key_len, attribute);
}

sentry_value_t
sentry__scope_get_attributes(const sentry_scope_t *scope)
{
    return data_get_attributes(scope->data);
}

void
sentry_scope_remove_attribute(sentry_scope_t *scope, const char *key)
{
    data_remove_attribute(scope->data, key);
}

void
sentry_scope_remove_attribute_n(
    sentry_scope_t *scope, const char *key, size_t key_len)
{
    data_remove_attribute_n(scope->data, key, key_len);
}

sentry_value_t
sentry__scope_get_contexts(const sentry_scope_t *scope)
{
    return data_get_contexts(scope->data);
}

void
sentry_scope_set_context(
    sentry_scope_t *scope, const char *key, sentry_value_t value)
{
    if (data_set_context(scope->data, key, sentry_value_incref(value))) {
        SENTRY_SCOPE_NOTIFY(scope, set_context, key, value);
    }
    sentry_value_decref(value);
}

void
sentry_scope_set_context_n(sentry_scope_t *scope, const char *key,
    size_t key_len, sentry_value_t value)
{
    char *notify_key = sentry__string_clone_n(key, key_len);
    if (!notify_key) {
        sentry_value_decref(value);
        return;
    }

    if (data_set_context_n(
            scope->data, key, key_len, sentry_value_incref(value))) {
        SENTRY_SCOPE_NOTIFY(scope, set_context, notify_key, value);
    }
    sentry_free(notify_key);
    sentry_value_decref(value);
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
    char *notify_key = sentry__string_clone_n(key, key_len);
    if (!notify_key) {
        sentry_value_decref(value);
        return;
    }

    if (data_update_context_n(
            scope->data, key, key_len, sentry_value_incref(value))) {
        SENTRY_SCOPE_NOTIFY(scope, set_context, notify_key, value);
    }
    sentry_free(notify_key);
    sentry_value_decref(value);
}

void
sentry__scope_remove_context(sentry_scope_t *scope, const char *key)
{
    if (data_remove_context(scope->data, key)) {
        SENTRY_SCOPE_NOTIFY(scope, remove_context, key);
    }
}

void
sentry__scope_remove_context_n(
    sentry_scope_t *scope, const char *key, size_t key_len)
{
    char *k = data_remove_context_n(scope->data, key, key_len);
    if (k) {
        SENTRY_SCOPE_NOTIFY(scope, remove_context, k);
    }
    sentry_free(k);
}

sentry_value_t
sentry__scope_ref_fingerprint(const sentry_scope_t *scope)
{
    return data_ref_fingerprint(scope->data);
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

    data_set_fingerprint(scope->data, fingerprints);
    SENTRY_SCOPE_NOTIFY_OWNED(scope, set_fingerprint, fingerprints);
}

void
sentry__scope_remove_fingerprint(sentry_scope_t *scope)
{
    sentry_value_t fingerprint = sentry_value_new_null();
    data_set_fingerprint(scope->data, fingerprint);
    SENTRY_SCOPE_NOTIFY_OWNED(scope, set_fingerprint, fingerprint);
}

sentry_level_t
sentry__scope_get_level(const sentry_scope_t *scope)
{
    return data_get_level(scope->data);
}

void
sentry_scope_set_level(sentry_scope_t *scope, sentry_level_t level)
{
    data_set_level(scope->data, level);
    SENTRY_SCOPE_NOTIFY(scope, set_level, level);
}

sentry_attachment_t *
sentry__scope_get_attachments(const sentry_scope_t *scope)
{
    return data_get_attachments(scope->data);
}

sentry_attachment_t *
sentry__scope_add_attachment(
    sentry_scope_t *scope, sentry_attachment_t *attachment)
{
    if (!attachment) {
        return NULL;
    }

    sentry_attachment_t *added = data_add_attachment(scope->data, attachment);
    if (added == attachment) {
        SENTRY_SCOPE_NOTIFY(scope, add_attachment, attachment);
    }
    return added;
}

bool
sentry__scope_remove_attachment(
    sentry_scope_t *scope, sentry_attachment_t *attachment)
{
    return data_remove_attachment(scope->data, attachment);
}

sentry_attachment_t *
sentry__scope_take_attachments(sentry_scope_t *scope)
{
    return data_take_attachments(scope->data);
}

sentry_transaction_t *
sentry__scope_get_transaction_object(const sentry_scope_t *scope)
{
    return data_get_transaction_object(scope->data);
}

void
sentry__scope_set_transaction_object(
    sentry_scope_t *scope, sentry_transaction_t *transaction)
{
    data_set_transaction_object(scope->data, transaction);
}

bool
sentry__scope_remove_transaction_object(
    sentry_scope_t *scope, sentry_transaction_t *transaction)
{
    return data_remove_transaction_object(scope->data, transaction);
}

bool
sentry__scope_remove_transaction_value(
    sentry_scope_t *scope, sentry_value_t transaction)
{
    return data_remove_transaction_value(scope->data, transaction);
}

bool
sentry__scope_restore_transaction_object(
    sentry_scope_t *scope, sentry_transaction_t *transaction)
{
    return data_restore_transaction_object(scope->data, transaction);
}

sentry_span_t *
sentry__scope_get_span(const sentry_scope_t *scope)
{
    return data_get_span(scope->data);
}

void
sentry__scope_set_span(sentry_scope_t *scope, sentry_span_t *span)
{
    data_set_span(scope->data, span);
}

bool
sentry__scope_remove_span(sentry_scope_t *scope, sentry_span_t *span)
{
    return data_remove_span(scope->data, span);
}

bool
sentry__scope_remove_span_value(sentry_scope_t *scope, sentry_value_t span)
{
    return data_remove_span_value(scope->data, span);
}

bool
sentry__scope_restore_span(sentry_scope_t *scope, sentry_span_t *span)
{
    return data_restore_span(scope->data, span);
}

sentry_value_t
sentry__scope_ref_release(const sentry_scope_t *scope)
{
    return data_ref_release(scope->data);
}

void
sentry__scope_set_release_n(
    sentry_scope_t *scope, const char *release, size_t release_len)
{
    sentry_value_t value = sentry_value_new_string_n(release, release_len);
    data_set_release(scope->data, value);
    SENTRY_SCOPE_NOTIFY_OWNED(scope, set_release, value);
}

sentry_value_t
sentry__scope_ref_environment(const sentry_scope_t *scope)
{
    return data_ref_environment(scope->data);
}

void
sentry__scope_set_environment_n(
    sentry_scope_t *scope, const char *environment, size_t environment_len)
{
    sentry_value_t value
        = sentry_value_new_string_n(environment, environment_len);
    data_set_environment(scope->data, value);
    SENTRY_SCOPE_NOTIFY_OWNED(scope, set_environment, value);
}

sentry_value_t
sentry__scope_ref_transaction(const sentry_scope_t *scope)
{
    return data_ref_transaction(scope->data);
}

void
sentry__scope_set_transaction_n(
    sentry_scope_t *scope, const char *transaction, size_t transaction_len)
{
    sentry_value_t value
        = sentry_value_new_string_n(transaction, transaction_len);
    data_set_transaction(scope->data, value, transaction, transaction_len);
    SENTRY_SCOPE_NOTIFY_OWNED(scope, set_transaction, value);
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
    data_apply_to_telemetry(scope->data, telemetry, attributes);

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

    sentry_value_t environment = sentry__scope_ref_environment(scope);
    if (!sentry_value_is_null(environment)) {
        sentry__value_add_attribute(attributes,
            sentry_value_incref(environment), "string", "sentry.environment");
    }
    sentry_value_decref(environment);

    sentry_value_t release = sentry__scope_ref_release(scope);
    if (!sentry_value_is_null(release)) {
        sentry__value_add_attribute(attributes, sentry_value_incref(release),
            "string", "sentry.release");
    }
    sentry_value_decref(release);
}
