#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_hint.h"
#include "sentry_options.h"
#include "sentry_scope.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"
#include "sentry_transport.h"
#include "transports/sentry_http_transport.h"

static void
send_envelope_test_basic(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_value_t event = sentry_envelope_get_event(envelope);
    TEST_CHECK(!sentry_value_is_null(event));
    const char *event_id
        = sentry_value_as_string(sentry_value_get_by_key(event, "event_id"));
    TEST_CHECK_STRING_EQUAL(event_id, "4c035723-8638-4c3a-923f-2ab9d08b4018");

    if (*called == 1) {
        const char *msg = sentry_value_as_string(sentry_value_get_by_key(
            sentry_value_get_by_key(event, "message"), "formatted"));
        TEST_CHECK_STRING_EQUAL(msg, "Hello World!");
        const char *release
            = sentry_value_as_string(sentry_value_get_by_key(event, "release"));
        TEST_CHECK_STRING_EQUAL(release, "prod");
        const char *trans = sentry_value_as_string(
            sentry_value_get_by_key(event, "transaction"));
        TEST_CHECK_STRING_EQUAL(trans, "demo-trans");
    }
    sentry_envelope_free(envelope);
}

SENTRY_TEST(basic_function_transport)
{
    uint64_t called = 0;
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport
        = sentry_transport_new(send_envelope_test_basic);
    sentry_transport_set_state(transport, &called);
    sentry_options_set_transport(options, transport);
    sentry_options_set_release(options, "prod");
    sentry_options_set_require_user_consent(options, true);
    sentry_init(options);

    sentry_set_transaction("demo-trans");

    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "root", "not captured due to missing consent"));
    sentry_user_consent_give();

    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "root", "Hello World!"));

    sentry_value_t obj = sentry_value_new_object();
    // something that is not a UUID, as this will be forcibly changed
    sentry_value_set_by_key(obj, "event_id", sentry_value_new_int32(1234));
    sentry_capture_event(obj);

    sentry_user_consent_revoke();
    sentry_capture_event(sentry_value_new_message_event(SENTRY_LEVEL_INFO,
        "root", "not captured either due to revoked consent"));

    sentry_close();

    TEST_CHECK_INT_EQUAL(called, 2);
}

static void
counting_transport_func(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;
    sentry_envelope_free(envelope);
}

static sentry_value_t
before_send(sentry_value_t event, sentry_hint_t *UNUSED(hint), void *data)
{
    uint64_t *called = data;
    *called += 1;

    return event;
}

SENTRY_TEST(sampling_before_send)
{
    uint64_t called_beforesend = 0;
    uint64_t called_transport = 0;

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_func);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);
    sentry_options_set_before_send(options, before_send, &called_beforesend);
    sentry_options_set_sample_rate(options, 0.75);
    sentry_init(options);

    for (int i = 0; i < 100; i++) {
        sentry_capture_event(
            sentry_value_new_message_event(SENTRY_LEVEL_INFO, NULL, "foo"));
    }

    sentry_close();

    // The behavior here has changed with version 0.4.19:
    // the documentation (https://develop.sentry.dev/sdk/sessions/#filter-order)
    // requires that the sampling-rate filter for all SDKs is executed last.
    // This means the `before_send` callback will be invoked every time and only
    // the actual transport will be randomly sampled.
    TEST_CHECK(called_transport > 50 && called_transport < 100);
    TEST_CHECK_INT_EQUAL(called_beforesend, 100);
}

static sentry_value_t
discarding_before_send(
    sentry_value_t event, sentry_hint_t *UNUSED(hint), void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_value_decref(event);
    return sentry_value_new_null();
}

SENTRY_TEST(discarding_before_send)
{
    uint64_t called_beforesend = 0;
    uint64_t called_transport = 0;

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    // Disable sessions or this test would fail if env:SENTRY_RELEASE is set.
    sentry_options_set_auto_session_tracking(options, 0);
    sentry_transport_t *transport
        = sentry_transport_new(counting_transport_func);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);
    sentry_options_set_before_send(
        options, discarding_before_send, &called_beforesend);
    sentry_init(options);

    sentry_capture_event(
        sentry_value_new_message_event(SENTRY_LEVEL_INFO, NULL, "foo"));

    sentry_close();

    TEST_CHECK_INT_EQUAL(called_transport, 0);
    TEST_CHECK_INT_EQUAL(called_beforesend, 1);
}

SENTRY_TEST(crash_marker)
{
    // We don't use sentry_init() in this test so we must create a database dir
    sentry_path_t *database_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".sentry-native");
    TEST_ASSERT(!!database_path);
    TEST_ASSERT(!sentry__path_create_dir_all(database_path));

    SENTRY_TEST_OPTIONS_NEW(options);
#ifdef SENTRY_PLATFORM_WINDOWS
    sentry_options_set_database_pathw(options, database_path->path_w);
#else
    sentry_options_set_database_path(options, database_path->path);
#endif

    // There is no marker in the beginning, but clearing returns true if the
    // marker doesn't exist (i.e., we get an `ENOENT` or `ERROR_FILE_NOT_FOUND`)
    TEST_CHECK(sentry__clear_crash_marker(options));
    // We can also verify this with has_crash_marker
    TEST_CHECK(!sentry__has_crash_marker(options));

    TEST_CHECK(sentry__write_crash_marker(options));
    TEST_CHECK(sentry__has_crash_marker(options));
    TEST_CHECK(sentry__write_crash_marker(options));
    TEST_CHECK(sentry__has_crash_marker(options));

    TEST_CHECK(sentry__clear_crash_marker(options));
    TEST_CHECK(!sentry__has_crash_marker(options));
    TEST_CHECK(sentry__clear_crash_marker(options));

    sentry_options_free(options);

    sentry__path_remove_all(database_path);
    sentry__path_free(database_path);
}

SENTRY_TEST(crashed_last_run)
{
    // fails before init() is called
    SENTRY_TEST_DEPRECATED(
        TEST_CHECK_INT_EQUAL(sentry_clear_crashed_last_run(), 1));

    // clear any leftover from previous test runs
    {
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry__clear_crash_marker(options);
        sentry_options_free(options);
    }

    const char dsn[] = { 'h', 't', 't', 'p', 's', ':', '/', '/', 'f', 'o', 'o',
        '@', 's', 'e', 'n', 't', 'r', 'y', '.', 'i', 'n', 'v', 'a', 'l', 'i',
        'd', '/', '4', '2' };

    {
        const char *dsn_str = "https://foo@sentry.invalid/42";
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_options_set_dsn_n(options, dsn, sizeof(dsn));
        TEST_CHECK_STRING_EQUAL(sentry_options_get_dsn(options), dsn_str);
        TEST_CHECK_INT_EQUAL(sentry_init(options), 0);
        sentry_close();

        TEST_CHECK_INT_EQUAL(sentry_get_crashed_last_run(), 0);
    }

    {
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_options_set_dsn_n(options, dsn, sizeof(dsn));

        // simulate a crash
        TEST_CHECK(sentry__write_crash_marker(options));

        TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

        TEST_CHECK_INT_EQUAL(sentry_get_crashed_last_run(), 1);
        TEST_CHECK(!sentry__has_crash_marker(options));

        sentry_close();

        // no change yet before sentry_init() is called
        TEST_CHECK_INT_EQUAL(sentry_get_crashed_last_run(), 1);
    }

    {
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_options_set_dsn_n(options, dsn, sizeof(dsn));

        // simulate a crash
        TEST_CHECK(sentry__write_crash_marker(options));

        sentry__retain_crash_marker(options);
        TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

        TEST_CHECK_INT_EQUAL(sentry_get_crashed_last_run(), 1);
        TEST_CHECK(sentry__has_crash_marker(options));
        // explicit clearing remains supported on all platforms
        SENTRY_TEST_DEPRECATED(
            TEST_CHECK_INT_EQUAL(sentry_clear_crashed_last_run(), 0));

        sentry_close();
    }

    {
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_options_set_dsn_n(options, dsn, sizeof(dsn));
        TEST_CHECK_INT_EQUAL(sentry_init(options), 0);
        sentry_close();

        TEST_CHECK_INT_EQUAL(sentry_get_crashed_last_run(), 0);
    }
}

SENTRY_TEST(capture_minidump_basic)
{
    // skipping on platforms that don't have access to fixtures on the local FS
#if defined(SENTRY_PLATFORM_ANDROID) || defined(SENTRY_PLATFORM_NX)            \
    || defined(SENTRY_PLATFORM_PS) || defined(SENTRY_PLATFORM_XBOX)
    SKIP_TEST();
#else
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    const char *minidump_rel_path = "../fixtures/minidump.dmp";
    sentry_path_t *path = sentry__path_from_str(__FILE__);
    sentry_path_t *dir = sentry__path_dir(path);
    sentry_path_t *minidump_path
        = sentry__path_join_str(dir, minidump_rel_path);

    const sentry_uuid_t event_id = sentry_capture_minidump(minidump_path->path);
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));
    sentry_uuid_t last_event_id = sentry_get_last_event_id();
    TEST_CHECK(memcmp(&last_event_id, &event_id, sizeof(sentry_uuid_t)) == 0);

    sentry__path_free(minidump_path);
    sentry__path_free(dir);
    sentry__path_free(path);

    sentry_close();
#endif
}

SENTRY_TEST(capture_minidump_wide)
{
#if !defined(SENTRY_PLATFORM_WINDOWS) || defined(SENTRY_PLATFORM_XBOX)
    SKIP_TEST();
#else
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    const wchar_t *minidump_rel_path = L"../fixtures/minidump.dmp";
    sentry_path_t *path = sentry__path_from_str(__FILE__);
    sentry_path_t *dir = sentry__path_dir(path);
    sentry_path_t *minidump_path
        = sentry__path_join_wstr(dir, minidump_rel_path);

    const sentry_uuid_t event_id
        = sentry_capture_minidumpw(minidump_path->path_w);
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));
    sentry_uuid_t last_event_id = sentry_get_last_event_id();
    TEST_CHECK(memcmp(&last_event_id, &event_id, sizeof(sentry_uuid_t)) == 0);

    sentry__path_free(minidump_path);
    sentry__path_free(dir);
    sentry__path_free(path);

    sentry_close();
#endif
}

SENTRY_TEST(capture_minidump_null_path)
{
    // a NULL path will activate the path check at the beginning of the function
    const sentry_uuid_t event_id = sentry_capture_minidump(NULL);
    TEST_CHECK(sentry_uuid_is_nil(&event_id));
}

SENTRY_TEST(capture_minidump_without_sentry_init)
{
    // if the path initialization was successful, but the SDK wasn't
    // initialized, capturing will fail at the point of acquiring the active
    // options.
    const sentry_uuid_t event_id
        = sentry_capture_minidump("irrelevant_minidump_path");
    TEST_CHECK(sentry_uuid_is_nil(&event_id));
}

SENTRY_TEST(capture_minidump_invalid_path)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    // here the initialization is successful, but we provide an invalid minidump
    // path which should prevent capture locally and return a nil UUID since we
    // cannot create an attachment envelope-item for the minidump file.
    const sentry_uuid_t event_id
        = sentry_capture_minidump("some_invalid_minidump_path");
    TEST_CHECK(sentry_uuid_is_nil(&event_id));

    sentry_close();
}

SENTRY_TEST(capture_minidump_discard)
{
    // skipping on platforms that don't have access to fixtures on the local FS
#if defined(SENTRY_PLATFORM_ANDROID) || defined(SENTRY_PLATFORM_NX)            \
    || defined(SENTRY_PLATFORM_PS) || defined(SENTRY_PLATFORM_XBOX)
    SKIP_TEST();
#else
    uint64_t called_beforesend = 0;

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_before_send(
        options, discarding_before_send, &called_beforesend);
    sentry_init(options);

    const char *minidump_rel_path = "../fixtures/minidump.dmp";
    sentry_path_t *path = sentry__path_from_str(__FILE__);
    sentry_path_t *dir = sentry__path_dir(path);
    sentry_path_t *minidump_path
        = sentry__path_join_str(dir, minidump_rel_path);

    const sentry_uuid_t event_id = sentry_capture_minidump(minidump_path->path);
    TEST_CHECK(sentry_uuid_is_nil(&event_id));
    TEST_CHECK_INT_EQUAL(called_beforesend, 1);

    sentry__path_free(minidump_path);
    sentry__path_free(dir);
    sentry__path_free(path);

    sentry_close();
#endif
}

SENTRY_TEST(basic_transport_thread_name)
{
#if defined(SENTRY_PLATFORM_NX)
    // NX transport won't start without custom network_connect_func.
    SKIP_TEST();
#endif

    const char *expected_thread_name = "sentry::worker_thread";

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport_thread_name(options, expected_thread_name);

    // Initialize sentry which should start the transport and set the thread
    // name
    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Access the transport through runtime options to check if thread name was
    // set
    SENTRY_WITH_OPTIONS (runtime_options) {
        TEST_ASSERT(!!runtime_options->transport);

        // Get the bgworker from the transport (for HTTP transports)
        sentry_bgworker_t *bgworker
            = (sentry_bgworker_t *)sentry__http_transport_get_bgworker(
                runtime_options->transport);
        TEST_ASSERT(!!bgworker);

        // Check if the thread name was properly set on the bgworker
        const char *actual_thread_name
            = sentry__bgworker_get_thread_name(bgworker);

        if (actual_thread_name) {
            TEST_CHECK_STRING_EQUAL(actual_thread_name, expected_thread_name);
        } else {
            TEST_CHECK(false); // Fail if thread_name is NULL
            TEST_MSG("Transport thread name was not set ");
        }
    }

    sentry_close();
}

SENTRY_TEST(client_sdk_integrations)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_integration_t *integration = SENTRY_MAKE(sentry_integration_t);
    TEST_ASSERT(!!integration);
    integration->name = "custom";
    TEST_ASSERT(sentry__options_add_integration(options, integration));

    sentry_init(options);

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t client_sdk = sentry__scope_ref_client_sdk(scope);
        sentry_value_t integrations
            = sentry_value_get_by_key(client_sdk, "integrations");
        size_t integration_count = sentry_value_get_length(integrations);
        TEST_CHECK(integration_count > 0);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(
                sentry_value_get_by_index(integrations, integration_count - 1)),
            "custom");
#if defined(SENTRY_INTEGRATION_WER) || defined(SENTRY_INTEGRATION_QT)
        size_t integration_index = integration_count - 1;
#endif
#ifdef SENTRY_INTEGRATION_WER
        integration_index--;
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(
                sentry_value_get_by_index(integrations, integration_index)),
            "wer");
#endif
#ifdef SENTRY_INTEGRATION_QT
        integration_index--;
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(
                sentry_value_get_by_index(integrations, integration_index)),
            "qt");
#endif
        sentry_value_decref(client_sdk);
    }

    sentry_close();
}

SENTRY_TEST(installation_id)
{
    // no DSN -> installation ID is generated
    SENTRY_TEST_OPTIONS_NEW(opts0);
    sentry_init(opts0);
    SENTRY_WITH_OPTIONS (options) {
        TEST_ASSERT(!!options->run->installation_id);
        TEST_CHECK_INT_EQUAL(strlen(options->run->installation_id), 36);
    }
    sentry_close();

    // DSN A -> installation ID is generated
    SENTRY_TEST_OPTIONS_NEW(opts1);
    sentry_options_set_dsn(opts1, "http://keya@127.0.0.1/42");
    sentry_init(opts1);
    char *id_a = NULL;
    SENTRY_WITH_OPTIONS (options) {
        TEST_ASSERT(!!options->run->installation_id);
        TEST_CHECK_INT_EQUAL(strlen(options->run->installation_id), 36);
        id_a = sentry__string_clone(options->run->installation_id);
    }
    sentry_close();

    // same DSN A -> installation ID persists
    SENTRY_TEST_OPTIONS_NEW(opts2);
    sentry_options_set_dsn(opts2, "http://keya@127.0.0.1/42");
    sentry_init(opts2);
    SENTRY_WITH_OPTIONS (options) {
        TEST_ASSERT(!!options->run->installation_id);
        TEST_CHECK_STRING_EQUAL(options->run->installation_id, id_a);
    }
    sentry_close();

    // different DSN B -> installation ID rotates
    SENTRY_TEST_OPTIONS_NEW(opts3);
    sentry_options_set_dsn(opts3, "http://keyb@127.0.0.1/42");
    sentry_init(opts3);
    SENTRY_WITH_OPTIONS (options) {
        TEST_ASSERT(!!options->run->installation_id);
        TEST_CHECK(strcmp(options->run->installation_id, id_a) != 0);
    }
    sentry_close();

    sentry_free(id_a);
}

static void
backend_free_options_ref(sentry_backend_t *backend)
{
    const sentry_options_t **options_ref = backend->data;
    *options_ref = sentry__options_getref();
}

SENTRY_TEST(clear_options)
{
    const sentry_options_t *options_ref = NULL;

    SENTRY_TEST_OPTIONS_NEW(options);

    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    TEST_ASSERT(!!backend);
    backend->free_func = backend_free_options_ref;
    backend->data = &options_ref;
    sentry_options_set_backend(options, backend);

    sentry_init(options);
    sentry_close();

    // The backend free hook runs from sentry_options_free(). At that point,
    // sentry__options_getref() must no longer expose the options.
    TEST_CHECK(options_ref == NULL);
}

static void
capture_envelope(sentry_envelope_t *envelope, void *data)
{
    sentry_envelope_t **captured = data;
    TEST_CHECK(*captured == NULL);
    *captured = envelope;
}

static sentry_value_t
attach_before_send(sentry_value_t event, sentry_hint_t *hint, void *data)
{
    TEST_CHECK(hint != NULL);
    if (data) {
        TEST_CHECK(hint == data);
    }
    sentry_hint_attach_bytes(hint, "callback", 8, "callback.txt");
    return event;
}

static sentry_value_t
discard_before_send(sentry_value_t event, sentry_hint_t *hint, void *data)
{
    TEST_CHECK(hint == data);
    sentry_value_decref(event);
    return sentry_value_new_null();
}

SENTRY_TEST(capture_event_hints)
{
    for (int mode = 0; mode < 4; mode++) {
        sentry_envelope_t *captured = NULL;
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_options_set_auto_session_tracking(options, false);
        sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
        sentry_transport_t *transport = sentry_transport_new(capture_envelope);
        sentry_transport_set_state(transport, &captured);
        sentry_options_set_transport(options, transport);
        sentry_hint_t *hint = mode ? sentry_hint_new() : NULL;
        if (hint) {
            sentry_hint_attach_bytes(hint, "hint", 4, "hint.txt");
        }
        sentry_options_set_before_send(options,
            mode == 3 ? discard_before_send : attach_before_send, hint);
        TEST_CHECK_INT_EQUAL(sentry_init(options), 0);
        sentry_attach_bytes("global", 6, "global.txt");
        sentry_uuid_t id;
        if (mode == 2) {
            sentry_scope_t *scope = sentry_scope_new();
            sentry_scope_attach_bytes(scope, "local", 5, "local.txt");
            id = sentry_scope_capture_event(
                scope, sentry_value_new_event(), hint);
            sentry_scope_free(scope);
        } else if (mode == 0) {
            id = sentry_capture_event(sentry_value_new_event());
        } else {
            id = sentry_scope_capture_event(
                NULL, sentry_value_new_event(), hint);
        }
        sentry_close();
        TEST_CHECK(sentry_uuid_is_nil(&id) == (mode == 3));
        if (mode == 3) {
            TEST_CHECK(captured == NULL);
            continue;
        }
        TEST_ASSERT(captured != NULL);
        size_t size;
        char *serialized = sentry_envelope_serialize(captured, &size);
        TEST_ASSERT(serialized != NULL);
        TEST_CHECK(strstr(serialized, "global.txt") != NULL);
        TEST_CHECK(strstr(serialized, "callback.txt") != NULL);
        TEST_CHECK((strstr(serialized, "hint.txt") != NULL) == (mode != 0));
        TEST_CHECK((strstr(serialized, "local.txt") != NULL) == (mode == 2));
        sentry_free(serialized);
        sentry_envelope_free(captured);
    }
}

SENTRY_TEST(capture_hint_cleanup)
{
    sentry_hint_t *hint = sentry_hint_new();
    sentry_hint_attach_bytes(hint, "hint", 4, "hint.txt");
    sentry_uuid_t id
        = sentry_scope_capture_event(NULL, sentry_value_new_event(), hint);
    TEST_CHECK(sentry_uuid_is_nil(&id));
    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(
        event, "type", sentry_value_new_string("transaction"));
    sentry_scope_t *scope = sentry_local_scope_new();
    id = sentry_scope_capture_event(scope, event, sentry_hint_new());
    TEST_CHECK(sentry_uuid_is_nil(&id));
    sentry_value_decref(event);
    sentry_scope_free(scope);
}

typedef struct {
    sentry_uuid_t global;
    sentry_uuid_t local;
    bool clear;
    bool discard;
    int calls;
} attachment_filter_t;

static sentry_value_t
filter_attachments(sentry_value_t event, sentry_hint_t *hint, void *data)
{
    attachment_filter_t *filter = data;
    filter->calls++;
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(hint->attachments), 3);
    sentry_hint_remove_attachment(hint, filter->global);
    sentry_hint_remove_attachment(hint, filter->local);
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(hint->attachments), 1);
    if (filter->clear) {
        sentry_hint_clear_attachments(hint);
    }
    sentry_hint_attach_bytes(hint, "callback", 8, "callback.txt");
    if (filter->discard) {
        sentry_value_decref(event);
        return sentry_value_new_null();
    }
    return event;
}

SENTRY_TEST(capture_filter_attachments)
{
    for (int feedback = 0; feedback < 2; feedback++) {
        for (int mode = 0; mode < 3; mode++) {
            sentry_envelope_t *captured = NULL;
            attachment_filter_t filter = { 0 };
            filter.clear = mode == 1;
            filter.discard = mode == 2;
            SENTRY_TEST_OPTIONS_NEW(options);
            sentry_options_set_auto_session_tracking(options, false);
            sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
            sentry_transport_t *transport
                = sentry_transport_new(capture_envelope);
            sentry_transport_set_state(transport, &captured);
            sentry_options_set_transport(options, transport);
            if (feedback) {
                sentry_options_set_before_send_feedback(
                    options, filter_attachments, &filter);
            } else {
                sentry_options_set_before_send(
                    options, filter_attachments, &filter);
            }
            TEST_CHECK_INT_EQUAL(sentry_init(options), 0);
            filter.global = sentry_attach_bytes("global", 6, "global.txt");
            sentry_scope_t *scope = sentry_scope_new();
            filter.local
                = sentry_scope_attach_bytes(scope, "local", 5, "local.txt");
            for (int capture = 0; capture < 2; capture++) {
                sentry_hint_t *hint = sentry_hint_new();
                sentry_hint_attach_bytes(hint, "hint", 4, "hint.txt");
                sentry_uuid_t id;
                if (feedback) {
                    id = sentry_scope_capture_feedback(scope,
                        sentry_value_new_feedback("message", NULL, NULL, NULL),
                        hint);
                } else {
                    id = sentry_scope_capture_event(
                        scope, sentry_value_new_event(), hint);
                }
                TEST_CHECK(sentry_uuid_is_nil(&id) == filter.discard);
                if (filter.discard) {
                    TEST_CHECK(captured == NULL);
                    continue;
                }
                TEST_ASSERT(captured != NULL);
                size_t size;
                char *serialized = sentry_envelope_serialize(captured, &size);
                TEST_ASSERT(serialized != NULL);
                TEST_CHECK(strstr(serialized, "global.txt") == NULL);
                TEST_CHECK(strstr(serialized, "local.txt") == NULL);
                TEST_CHECK(
                    (strstr(serialized, "hint.txt") == NULL) == filter.clear);
                TEST_CHECK(strstr(serialized, "callback.txt") != NULL);
                sentry_free(serialized);
                sentry_envelope_free(captured);
                captured = NULL;
            }
            TEST_CHECK_INT_EQUAL(filter.calls, 2);
            sentry_scope_free(scope);
            sentry_close();
        }
    }
}
