#include "sentry.h"
#include "sentry_database.h"
#include "sentry_options.h"
#include "sentry_scope.h"
#include "sentry_testsupport.h"
#include "sentry_utils.h"

SENTRY_TEST(scope_contexts)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

#define TEST_CHECK_CONTEXT_EQUAL(event, key, value)                            \
    do {                                                                       \
        sentry_value_t contexts = sentry_value_get_by_key(event, "contexts");  \
        TEST_CHECK_STRING_EQUAL(                                               \
            sentry_value_as_string(sentry_value_get_by_key(contexts, key)),    \
            value);                                                            \
    } while (0)

    // global:
    // {"all":"global","scope":"global","global":"global"}
    sentry_set_context("all", sentry_value_new_string("global"));
    sentry_set_context("global", sentry_value_new_string("global"));
    sentry_set_context("scope", sentry_value_new_string("global"));

    SENTRY_WITH_SCOPE (global_scope) {
        // event:
        // {"all":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t contexts = sentry_value_new_object();
            sentry_value_set_by_key(
                contexts, "all", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                contexts, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "contexts", contexts);
        }

        // event <- global:
        // {"all":"event","event":"event","global":"global","scope":"global"}
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_CONTEXT_EQUAL(event, "all", "event");
        TEST_CHECK_CONTEXT_EQUAL(event, "event", "event");
        TEST_CHECK_CONTEXT_EQUAL(event, "global", "global");
        TEST_CHECK_CONTEXT_EQUAL(event, "scope", "global");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local:
        // {"all":"scope","scope":"scope","local":"local"}
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry_scope_set_context(
            local_scope, "all", sentry_value_new_string("local"));
        sentry_scope_set_context(
            local_scope, "local", sentry_value_new_string("local"));
        sentry_scope_set_context(
            local_scope, "scope", sentry_value_new_string("local"));

        // event:
        // {"all":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t contexts = sentry_value_new_object();
            sentry_value_set_by_key(
                contexts, "all", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                contexts, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "contexts", contexts);
        }

        // event <- local:
        // {"all":"event","event":"event","local":"local","scope":"local"}
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_CONTEXT_EQUAL(event, "all", "event");
        TEST_CHECK_CONTEXT_EQUAL(event, "event", "event");
        TEST_CHECK_CONTEXT_EQUAL(event, "local", "local");
        TEST_CHECK_CONTEXT_EQUAL(event, "scope", "local");

        // event <- global:
        // {"all":"event","event":"event","global":"global","local":"local","scope":"local"}
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_CONTEXT_EQUAL(event, "all", "event");
        TEST_CHECK_CONTEXT_EQUAL(event, "event", "event");
        TEST_CHECK_CONTEXT_EQUAL(event, "global", "global");
        TEST_CHECK_CONTEXT_EQUAL(event, "local", "local");
        TEST_CHECK_CONTEXT_EQUAL(event, "scope", "local");

        sentry__scope_free(local_scope);
        sentry_value_decref(event);
    }

#undef TEST_CHECK_CONTEXT_EQUAL

    sentry_close();
}

SENTRY_TEST(scope_update_context)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    // update into empty: context does not exist yet
    {
        sentry_value_t device = sentry_value_new_object();
        sentry_value_set_by_key(
            device, "model", sentry_value_new_string("Xbox Series X"));
        sentry_value_set_by_key(
            device, "family", sentry_value_new_string("Xbox"));
        sentry_update_context("device", device);

        SENTRY_WITH_SCOPE (scope) {
            sentry_value_t ctx
                = sentry_value_get_by_key(scope->contexts, "device");
            TEST_CHECK_STRING_EQUAL(
                sentry_value_as_string(sentry_value_get_by_key(ctx, "model")),
                "Xbox Series X");
            TEST_CHECK_STRING_EQUAL(
                sentry_value_as_string(sentry_value_get_by_key(ctx, "family")),
                "Xbox");
        }
    }

    // update should overwrite existing keys
    {
        sentry_value_t extra = sentry_value_new_object();
        sentry_value_set_by_key(extra, "model", sentry_value_new_string("PC"));
        sentry_value_set_by_key(
            extra, "cpu_description", sentry_value_new_string("some cpu"));
        sentry_update_context("device", extra);

        SENTRY_WITH_SCOPE (scope) {
            sentry_value_t ctx
                = sentry_value_get_by_key(scope->contexts, "device");
            TEST_CHECK_STRING_EQUAL(
                sentry_value_as_string(sentry_value_get_by_key(ctx, "model")),
                "PC");
            TEST_CHECK_STRING_EQUAL(
                sentry_value_as_string(sentry_value_get_by_key(ctx, "family")),
                "Xbox");
            TEST_CHECK_STRING_EQUAL(
                sentry_value_as_string(
                    sentry_value_get_by_key(ctx, "cpu_description")),
                "some cpu");
        }
    }

    // scoped update into empty
    {
        sentry_scope_t *local_scope = sentry_local_scope_new();

        sentry_value_t os = sentry_value_new_object();
        sentry_value_set_by_key(os, "name", sentry_value_new_string("SteamOS"));
        sentry_scope_update_context(local_scope, "os", os);

        sentry_value_t ctx
            = sentry_value_get_by_key(local_scope->contexts, "os");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(ctx, "name")),
            "SteamOS");

        // scoped update overwrites existing keys
        sentry_value_t os2 = sentry_value_new_object();
        sentry_value_set_by_key(os2, "name", sentry_value_new_string("Linux"));
        sentry_value_set_by_key(os2, "version", sentry_value_new_string("6.1"));
        sentry_scope_update_context(local_scope, "os", os2);

        ctx = sentry_value_get_by_key(local_scope->contexts, "os");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(ctx, "name")),
            "Linux");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(ctx, "version")),
            "6.1");

        sentry__scope_free(local_scope);
    }

    sentry_close();
}

SENTRY_TEST(scope_propagation_context)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_set_trace("aaaabbbbccccddddeeeeffff00001111", "9c2a92b49b7c4d1e");

#define TEST_CHECK_EVENT_TRACE_ID_EQUAL(event, value)                          \
    do {                                                                       \
        sentry_value_t trace = sentry_value_get_by_key(                        \
            sentry_value_get_by_key(event, "contexts"), "trace");              \
        TEST_CHECK_STRING_EQUAL(                                               \
            sentry_value_as_string(                                            \
                sentry_value_get_by_key(trace, "trace_id")),                   \
            value);                                                            \
    } while (0)

    SENTRY_WITH_SCOPE (global_scope) {
        // event without contexts receives the trace from propagation
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_EVENT_TRACE_ID_EQUAL(
            event, "aaaabbbbccccddddeeeeffff00001111");
        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // event with pre-existing contexts still receives the trace
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t contexts = sentry_value_new_object();
            sentry_value_set_by_key(
                contexts, "custom", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "contexts", contexts);
        }
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_EVENT_TRACE_ID_EQUAL(
            event, "aaaabbbbccccddddeeeeffff00001111");
        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local scope applied first must not prevent the global scope
        // from adding the trace context afterwards
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_NONE);
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_EVENT_TRACE_ID_EQUAL(
            event, "aaaabbbbccccddddeeeeffff00001111");
        sentry__scope_free(local_scope);
        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // event that carries its own trace context keeps it
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t trace = sentry_value_new_object();
            sentry_value_set_by_key(trace, "trace_id",
                sentry_value_new_string("11112222333344445555666677778888"));
            sentry_value_t contexts = sentry_value_new_object();
            sentry_value_set_by_key(contexts, "trace", trace);
            sentry_value_set_by_key(event, "contexts", contexts);
        }
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_EVENT_TRACE_ID_EQUAL(
            event, "11112222333344445555666677778888");
        sentry_value_decref(event);
    }

#undef TEST_CHECK_EVENT_TRACE_ID_EQUAL

    sentry_close();
}

SENTRY_TEST(scope_extra)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

#define TEST_CHECK_EXTRA_EQUAL(event, key, value)                              \
    do {                                                                       \
        sentry_value_t extra = sentry_value_get_by_key(event, "extra");        \
        TEST_CHECK_STRING_EQUAL(                                               \
            sentry_value_as_string(sentry_value_get_by_key(extra, key)),       \
            value);                                                            \
    } while (0)

    // global:
    // {"all":"global","scope":"global","global":"global"}
    sentry_set_extra("all", sentry_value_new_string("global"));
    sentry_set_extra("global", sentry_value_new_string("global"));
    sentry_set_extra("scope", sentry_value_new_string("global"));

    SENTRY_WITH_SCOPE (global_scope) {
        // event:
        // {"all":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t extra = sentry_value_new_object();
            sentry_value_set_by_key(
                extra, "all", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                extra, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "extra", extra);
        }

        // event <- global:
        // {"all":"event","event":"event","global":"global","scope":"global"}
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_EXTRA_EQUAL(event, "all", "event");
        TEST_CHECK_EXTRA_EQUAL(event, "event", "event");
        TEST_CHECK_EXTRA_EQUAL(event, "global", "global");
        TEST_CHECK_EXTRA_EQUAL(event, "scope", "global");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local:
        // {"all":"scope","scope":"scope","local":"local"}
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry_scope_set_extra(
            local_scope, "all", sentry_value_new_string("local"));
        sentry_scope_set_extra(
            local_scope, "local", sentry_value_new_string("local"));
        sentry_scope_set_extra(
            local_scope, "scope", sentry_value_new_string("local"));

        // event:
        // {"all":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t extra = sentry_value_new_object();
            sentry_value_set_by_key(
                extra, "all", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                extra, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "extra", extra);
        }

        // event <- local:
        // {"all":"event","event":"event","local":"local","scope":"local"}
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_EXTRA_EQUAL(event, "all", "event");
        TEST_CHECK_EXTRA_EQUAL(event, "event", "event");
        TEST_CHECK_EXTRA_EQUAL(event, "local", "local");
        TEST_CHECK_EXTRA_EQUAL(event, "scope", "local");

        // event <- global:
        // {"all":"event","event":"event","global":"global","local":"local","scope":"local"}
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_EXTRA_EQUAL(event, "all", "event");
        TEST_CHECK_EXTRA_EQUAL(event, "event", "event");
        TEST_CHECK_EXTRA_EQUAL(event, "global", "global");
        TEST_CHECK_EXTRA_EQUAL(event, "local", "local");
        TEST_CHECK_EXTRA_EQUAL(event, "scope", "local");

        sentry__scope_free(local_scope);
        sentry_value_decref(event);
    }

#undef TEST_CHECK_EXTRA_EQUAL

    sentry_close();
}

SENTRY_TEST(scope_fingerprint)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    // global:
    // ["global1", "global2"]
    sentry_set_fingerprint("global1", "global2", NULL);

    SENTRY_WITH_SCOPE (global_scope) {
        // event:
        // null
        sentry_value_t event = sentry_value_new_object();

        // event <- global:
        // ["global1", "global2"]
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_JSON_VALUE(sentry_value_get_by_key(event, "fingerprint"),
            "[\"global1\",\"global2\"]");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // event:
        // ["event1", "event2"]
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t fingerprint = sentry_value_new_list();
            sentry_value_append(fingerprint, sentry_value_new_string("event1"));
            sentry_value_append(fingerprint, sentry_value_new_string("event2"));
            sentry_value_set_by_key(event, "fingerprint", fingerprint);
        }

        // event <- global:
        // ["event1", "event2"]
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_JSON_VALUE(sentry_value_get_by_key(event, "fingerprint"),
            "[\"event1\",\"event2\"]");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local:
        // ["local1", "local2"]
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry_scope_set_fingerprint(local_scope, "local1", "local2", NULL);

        // event:
        // ["event1", "event2"]
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t fingerprint = sentry_value_new_list();
            sentry_value_append(fingerprint, sentry_value_new_string("event1"));
            sentry_value_append(fingerprint, sentry_value_new_string("event2"));
            sentry_value_set_by_key(event, "fingerprint", fingerprint);
        }

        // event <- local:
        // ["event1", "event2"]
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_JSON_VALUE(sentry_value_get_by_key(event, "fingerprint"),
            "[\"event1\",\"event2\"]");

        // event <- global:
        // ["event1", "event2"]
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_JSON_VALUE(sentry_value_get_by_key(event, "fingerprint"),
            "[\"event1\",\"event2\"]");

        sentry__scope_free(local_scope);
        sentry_value_decref(event);
    }

    sentry_close();
}

SENTRY_TEST(scope_fingerprint_n)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_set_fingerprint_n(
        "alphabet", (size_t)5, "0123456789", (size_t)8, "xyz", (size_t)1, NULL);

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t event = sentry_value_new_object();

        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_JSON_VALUE(sentry_value_get_by_key(event, "fingerprint"),
            "[\"alpha\",\"01234567\",\"x\"]");

        sentry_value_decref(event);
    }

    sentry_close();
}

SENTRY_TEST(scope_tags)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

#define TEST_CHECK_TAG_EQUAL(event, key, value)                                \
    do {                                                                       \
        sentry_value_t tags = sentry_value_get_by_key(event, "tags");          \
        TEST_CHECK_STRING_EQUAL(                                               \
            sentry_value_as_string(sentry_value_get_by_key(tags, key)),        \
            value);                                                            \
    } while (0)

    // global:
    // {"all":"global","scope":"global","global":"global"}
    sentry_set_tag("all", "global");
    sentry_set_tag("global", "global");
    sentry_set_tag("scope", "global");

    SENTRY_WITH_SCOPE (global_scope) {
        // event:
        // {"all":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t event_tags = sentry_value_new_object();
            sentry_value_set_by_key(
                event_tags, "all", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                event_tags, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "tags", event_tags);
        }

        // event <- global:
        // {"all":"event","event":"event","global":"global","scope":"global"}
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_TAG_EQUAL(event, "all", "event");
        TEST_CHECK_TAG_EQUAL(event, "event", "event");
        TEST_CHECK_TAG_EQUAL(event, "global", "global");
        TEST_CHECK_TAG_EQUAL(event, "scope", "global");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local:
        // {"all":"scope","scope":"scope","local":"local"}
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry_scope_set_tag(local_scope, "all", "local");
        sentry_scope_set_tag(local_scope, "local", "local");
        sentry_scope_set_tag(local_scope, "scope", "local");

        // event:
        // {"all":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t event_tags = sentry_value_new_object();
            sentry_value_set_by_key(
                event_tags, "all", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                event_tags, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "tags", event_tags);
        }

        // event <- local:
        // {"all":"event","event":"event","local":"local","scope":"local"}
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_TAG_EQUAL(event, "all", "event");
        TEST_CHECK_TAG_EQUAL(event, "event", "event");
        TEST_CHECK_TAG_EQUAL(event, "local", "local");
        TEST_CHECK_TAG_EQUAL(event, "scope", "local");

        // event <- global:
        // {"all":"event","event":"event","global":"global","local":"local","scope":"local"}
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_TAG_EQUAL(event, "all", "event");
        TEST_CHECK_TAG_EQUAL(event, "event", "event");
        TEST_CHECK_TAG_EQUAL(event, "global", "global");
        TEST_CHECK_TAG_EQUAL(event, "local", "local");
        TEST_CHECK_TAG_EQUAL(event, "scope", "local");

        sentry__scope_free(local_scope);
        sentry_value_decref(event);
    }

#undef TEST_CHECK_TAG_EQUAL

    sentry_close();
}

SENTRY_TEST(scope_user)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    // global: {"id":"1","username":"global","email":"@global"}
    sentry_set_user(sentry_value_new_user("1", "global", "@global", NULL));

    SENTRY_WITH_SCOPE (global_scope) {
        // event: {"id":"2","username":"event"}
        sentry_value_t event = sentry_value_new_object();
        sentry_value_set_by_key(
            event, "user", sentry_value_new_user("2", "event", NULL, NULL));

        // event <- global: {"id":"2","username":"event"}
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_JSON_VALUE(sentry_value_get_by_key(event, "user"),
            "{\"id\":\"2\",\"username\":\"event\"}");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local: {"id":"2","username":"local","email":"@local"}
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry_scope_set_user(
            local_scope, sentry_value_new_user("2", "local", "@local", NULL));

        // event: {"id":"3","username":"event"}
        sentry_value_t event = sentry_value_new_object();
        sentry_value_set_by_key(
            event, "user", sentry_value_new_user("3", "event", NULL, NULL));

        // event <- local: {"id":"3","username":"event"}
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_JSON_VALUE(sentry_value_get_by_key(event, "user"),
            "{\"id\":\"3\",\"username\":\"event\"}");

        // event <- local: {"id":"3","username":"event"}
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_JSON_VALUE(sentry_value_get_by_key(event, "user"),
            "{\"id\":\"3\",\"username\":\"event\"}");

        sentry__scope_free(local_scope);
        sentry_value_decref(event);
    }

    sentry_close();
}

SENTRY_TEST(scope_user_id)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "http://keya@127.0.0.1/42");
    sentry_init(options);

    // null user ID -> installation ID
    TEST_ASSERT(!!options->run->installation_id);
    TEST_CHECK_INT_EQUAL(strlen(options->run->installation_id), 36);
    sentry_set_user(sentry_value_new_user(NULL, "alice", NULL, NULL));
    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
        sentry_value_t user = sentry_value_get_by_key(event, "user");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(user, "id")),
            options->run->installation_id);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(user, "username")),
            "alice");
        sentry_value_decref(event);
    }

    // empty user ID
    sentry_set_user(sentry_value_new_user("", "bob", NULL, NULL));
    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
        sentry_value_t user = sentry_value_get_by_key(event, "user");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(user, "id")), "");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(user, "username")),
            "bob");
        sentry_value_decref(event);
    }

    // non-empty user ID
    sentry_set_user(sentry_value_new_user("42", "carol", NULL, NULL));
    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
        sentry_value_t user = sentry_value_get_by_key(event, "user");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(user, "id")), "42");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(user, "username")),
            "carol");
        sentry_value_decref(event);
    }

    // empty local scope does not shadow global user ID
    sentry_set_user(sentry_value_new_user("12345", "dave", NULL, NULL));
    sentry_value_t event = sentry_value_new_object();
    sentry_scope_t *local_scope = sentry_local_scope_new();
    sentry__scope_apply_to_event(
        local_scope, options, event, SENTRY_SCOPE_BREADCRUMBS);
    sentry__scope_free(local_scope);
    SENTRY_WITH_SCOPE (scope) {
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_ALL);
    }
    sentry_value_t user = sentry_value_get_by_key(event, "user");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user, "id")), "12345");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(user, "username")),
        "dave");
    sentry_value_decref(event);

    // remove_user -> no user on event (installation ID suppressed)
    sentry_remove_user();
    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK(
            sentry_value_is_null(sentry_value_get_by_key(event, "user")));
        sentry_value_decref(event);
    }

    sentry_close();
}

SENTRY_TEST(scope_level)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

#define TEST_CHECK_LEVEL_EQUAL(event, value)                                   \
    do {                                                                       \
        sentry_value_t level = sentry_value_get_by_key(event, "level");        \
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(level), value);         \
    } while (0)

    // global: warning
    sentry_set_level(SENTRY_LEVEL_WARNING);

    SENTRY_WITH_SCOPE (global_scope) {
        // event: null
        sentry_value_t event = sentry_value_new_object();

        // event <- global: warning
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_LEVEL_EQUAL(event, "warning");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // event: info
        sentry_value_t event = sentry_value_new_object();
        sentry_value_set_by_key(
            event, "level", sentry_value_new_string("info"));

        // event <- global: info
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_LEVEL_EQUAL(event, "info");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local: fatal
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry_scope_set_level(local_scope, SENTRY_LEVEL_FATAL);

        // event: debug
        sentry_value_t event = sentry_value_new_object();
        sentry_value_set_by_key(
            event, "level", sentry_value_new_string("debug"));

        // event <- local: debug
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_LEVEL_EQUAL(event, "debug");

        // event <- global: debug
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_LEVEL_EQUAL(event, "debug");

        sentry__scope_free(local_scope);
        sentry_value_decref(event);
    }

#undef TEST_CHECK_LEVEL_EQUAL

    sentry_close();
}

static sentry_value_t
breadcrumb_ts(const char *message, uint64_t ts)
{
    sentry_value_t breadcrumb = sentry_value_new_breadcrumb(NULL, message);
    sentry_value_set_by_key(breadcrumb, "timestamp",
        sentry__value_new_string_owned(sentry__usec_time_to_iso8601(ts)));
    return breadcrumb;
}

SENTRY_TEST(scope_breadcrumbs)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_max_breadcrumbs(options, 5);
    sentry_init(options);

    // global: ["global1", "global4"]
    sentry_add_breadcrumb(breadcrumb_ts("global1", 1));
    sentry_add_breadcrumb(breadcrumb_ts("global4", 4));

#define TEST_CHECK_MESSAGE_EQUAL(breadcrumbs, index, message)                  \
    TEST_CHECK_STRING_EQUAL(                                                   \
        sentry_value_as_string(sentry_value_get_by_key(                        \
            sentry_value_get_by_index(breadcrumbs, index), "message")),        \
        message)

    SENTRY_WITH_SCOPE (global_scope) {
        // event: null
        sentry_value_t event = sentry_value_new_object();

        // event <- global: ["global1", "global4"]
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_BREADCRUMBS);

        sentry_value_t result = sentry_value_get_by_key(event, "breadcrumbs");
        TEST_CHECK(sentry_value_get_type(result) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK_INT_EQUAL(sentry_value_get_length(result), 2);
        TEST_CHECK_MESSAGE_EQUAL(result, 0, "global1");
        TEST_CHECK_MESSAGE_EQUAL(result, 1, "global4");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // event: ["event3", "event5"]
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t breadcrumbs = sentry_value_new_list();
            sentry_value_append(breadcrumbs, breadcrumb_ts("event3", 3));
            sentry_value_append(breadcrumbs, breadcrumb_ts("event5", 5));
            sentry_value_set_by_key(event, "breadcrumbs", breadcrumbs);
        }

        // event <- global: ["global1", "event3", "global4", "event5"]
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_BREADCRUMBS);

        sentry_value_t result = sentry_value_get_by_key(event, "breadcrumbs");
        TEST_CHECK(sentry_value_get_type(result) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK_INT_EQUAL(sentry_value_get_length(result), 4);
        TEST_CHECK_MESSAGE_EQUAL(result, 0, "global1");
        TEST_CHECK_MESSAGE_EQUAL(result, 1, "event3");
        TEST_CHECK_MESSAGE_EQUAL(result, 2, "global4");
        TEST_CHECK_MESSAGE_EQUAL(result, 3, "event5");

        sentry_value_decref(event);
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local: ["local2", "local6"]
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry_scope_add_breadcrumb(local_scope, breadcrumb_ts("local2", 2));
        sentry_scope_add_breadcrumb(local_scope, breadcrumb_ts("local6", 6));

        // event: ["event3", "event5"]
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t breadcrumbs = sentry_value_new_list();
            sentry_value_append(breadcrumbs, breadcrumb_ts("event3", 3));
            sentry_value_append(breadcrumbs, breadcrumb_ts("event5", 5));
            sentry_value_set_by_key(event, "breadcrumbs", breadcrumbs);
        }

        // event <- local: ["local2", "event3", "event5", "local6"]
        sentry__scope_apply_to_event(
            local_scope, options, event, SENTRY_SCOPE_BREADCRUMBS);

        sentry_value_t result = sentry_value_get_by_key(event, "breadcrumbs");
        TEST_CHECK(sentry_value_get_type(result) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK_INT_EQUAL(sentry_value_get_length(result), 4);
        TEST_CHECK_MESSAGE_EQUAL(result, 0, "local2");
        TEST_CHECK_MESSAGE_EQUAL(result, 1, "event3");
        TEST_CHECK_MESSAGE_EQUAL(result, 2, "event5");
        TEST_CHECK_MESSAGE_EQUAL(result, 3, "local6");

        // event <- global: ["local2", "event3", "global4", "event5", "local6"]
        sentry__scope_apply_to_event(
            global_scope, options, event, SENTRY_SCOPE_BREADCRUMBS);

        result = sentry_value_get_by_key(event, "breadcrumbs");
        TEST_CHECK(sentry_value_get_type(result) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK_INT_EQUAL(sentry_value_get_length(result), 5);
        TEST_CHECK_MESSAGE_EQUAL(result, 0, "local2");
        TEST_CHECK_MESSAGE_EQUAL(result, 1, "event3");
        TEST_CHECK_MESSAGE_EQUAL(result, 2, "global4");
        TEST_CHECK_MESSAGE_EQUAL(result, 3, "event5");
        TEST_CHECK_MESSAGE_EQUAL(result, 4, "local6");

        sentry__scope_free(local_scope);
        sentry_value_decref(event);
    }

    sentry_close();
}

static sentry_value_t
before_breadcrumb_discard_cb(sentry_value_t breadcrumb, void *data)
{
    (void)data;
    sentry_value_decref(breadcrumb);
    return sentry_value_new_null();
}

SENTRY_TEST(before_breadcrumb_discard)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_breadcrumb(
        options, before_breadcrumb_discard_cb, NULL);
    sentry_init(options);

    sentry_add_breadcrumb(sentry_value_new_breadcrumb(NULL, "discarded"));

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(
            scope, options, event, SENTRY_SCOPE_BREADCRUMBS);

        sentry_value_t breadcrumbs
            = sentry_value_get_by_key(event, "breadcrumbs");
        TEST_CHECK(sentry_value_is_null(breadcrumbs)
            || sentry_value_get_length(breadcrumbs) == 0);

        sentry_value_decref(event);
    }

    sentry_close();
}

static sentry_value_t
before_breadcrumb_modify_cb(sentry_value_t breadcrumb, void *data)
{
    (void)data;
    sentry_value_set_by_key(
        breadcrumb, "message", sentry_value_new_string("modified"));
    return breadcrumb;
}

SENTRY_TEST(before_breadcrumb_modify)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_breadcrumb(
        options, before_breadcrumb_modify_cb, NULL);
    sentry_init(options);

    sentry_add_breadcrumb(sentry_value_new_breadcrumb(NULL, "original"));

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(
            scope, options, event, SENTRY_SCOPE_BREADCRUMBS);

        sentry_value_t breadcrumbs
            = sentry_value_get_by_key(event, "breadcrumbs");
        TEST_CHECK_INT_EQUAL(sentry_value_get_length(breadcrumbs), 1);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_index(breadcrumbs, 0), "message")),
            "modified");

        sentry_value_decref(event);
    }

    sentry_close();
}

static sentry_value_t
before_breadcrumb_passthrough_cb(sentry_value_t breadcrumb, void *data)
{
    (void)data;
    return breadcrumb;
}

SENTRY_TEST(before_breadcrumb_passthrough)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_breadcrumb(
        options, before_breadcrumb_passthrough_cb, NULL);
    sentry_init(options);

    sentry_add_breadcrumb(sentry_value_new_breadcrumb(NULL, "kept"));

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t event = sentry_value_new_object();
        sentry__scope_apply_to_event(
            scope, options, event, SENTRY_SCOPE_BREADCRUMBS);

        sentry_value_t breadcrumbs
            = sentry_value_get_by_key(event, "breadcrumbs");
        TEST_CHECK_INT_EQUAL(sentry_value_get_length(breadcrumbs), 1);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_index(breadcrumbs, 0), "message")),
            "kept");

        sentry_value_decref(event);
    }

    sentry_close();
}

SENTRY_TEST(scope_global_attributes)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    // Test setting a valid attribute on the global scope
    sentry_value_t valid_attr = sentry_value_new_attribute(
        sentry_value_new_string("test_value"), NULL);
    sentry_set_attribute("valid_key", valid_attr);

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t attributes = scope->attributes;
        sentry_value_t retrieved_attr
            = sentry_value_get_by_key(attributes, "valid_key");

        // Check that the attribute was set
        TEST_CHECK(!sentry_value_is_null(retrieved_attr));
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                    retrieved_attr, "type")),
            "string");
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                    retrieved_attr, "value")),
            "test_value");
    }

    // Test that invalid attributes (missing 'value' or 'type') are not set
    sentry_value_t invalid_attr_no_value = sentry_value_new_object();
    sentry_value_set_by_key(
        invalid_attr_no_value, "type", sentry_value_new_string("string"));
    // Missing 'value' field
    sentry_set_attribute("invalid_no_value", invalid_attr_no_value);

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t attributes = scope->attributes;
        sentry_value_t retrieved_attr
            = sentry_value_get_by_key(attributes, "invalid_no_value");

        // Check that the attribute was NOT set
        TEST_CHECK(sentry_value_is_null(retrieved_attr));
    }
    sentry_value_decref(invalid_attr_no_value);

    // Test invalid attribute missing 'type'
    sentry_value_t invalid_attr_no_type = sentry_value_new_object();
    sentry_value_set_by_key(
        invalid_attr_no_type, "value", sentry_value_new_string("some_value"));
    // Missing 'type' field
    sentry_set_attribute("invalid_no_type", invalid_attr_no_type);

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t attributes = scope->attributes;
        sentry_value_t retrieved_attr
            = sentry_value_get_by_key(attributes, "invalid_no_type");

        // Check that the attribute was NOT set
        TEST_CHECK(sentry_value_is_null(retrieved_attr));
    }
    sentry_value_decref(invalid_attr_no_type);

    // Test removing an attribute
    sentry_remove_attribute("valid_key");

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t attributes = scope->attributes;
        sentry_value_t retrieved_attr
            = sentry_value_get_by_key(attributes, "valid_key");

        // Check that the attribute was removed
        TEST_CHECK(sentry_value_is_null(retrieved_attr));
    }

    // Test setting attribute with _n variant
    sentry_value_t attr_n
        = sentry_value_new_attribute(sentry_value_new_int32(42), "percent");
    sentry_set_attribute_n("key_n", 5, attr_n);

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t attributes = scope->attributes;
        sentry_value_t retrieved_attr
            = sentry_value_get_by_key(attributes, "key_n");

        // Check that the attribute was set
        TEST_CHECK(!sentry_value_is_null(retrieved_attr));
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                    retrieved_attr, "type")),
            "integer");
        TEST_CHECK(sentry_value_as_int32(
                       sentry_value_get_by_key(retrieved_attr, "value"))
            == 42);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                    retrieved_attr, "unit")),
            "percent");
    }

    sentry_close();
}

SENTRY_TEST(scope_local_attributes)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    // global:
    // {"all":"global","scope":"global","global":"global"}
    sentry_set_attribute("all",
        sentry_value_new_attribute(sentry_value_new_string("global"), NULL));
    sentry_set_attribute("global",
        sentry_value_new_attribute(sentry_value_new_string("global"), NULL));
    sentry_set_attribute("scope",
        sentry_value_new_attribute(sentry_value_new_string("global"), NULL));

    SENTRY_WITH_SCOPE (global_scope) {
        sentry_value_t attributes = global_scope->attributes;

        // Verify global attributes are set
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(attributes, "all"), "value")),
            "global");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(attributes, "global"), "value")),
            "global");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(attributes, "scope"), "value")),
            "global");
    }

    SENTRY_WITH_SCOPE (global_scope) {
        // local:
        // {"all":"local","scope":"local","local":"local"}
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry__scope_set_attribute(local_scope, "all",
            sentry_value_new_attribute(sentry_value_new_string("local"), NULL));
        sentry__scope_set_attribute(local_scope, "local",
            sentry_value_new_attribute(sentry_value_new_string("local"), NULL));
        sentry__scope_set_attribute(local_scope, "scope",
            sentry_value_new_attribute(sentry_value_new_string("local"), NULL));

        sentry_value_t local_attributes = local_scope->attributes;

        // Verify local attributes are set
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(local_attributes, "all"), "value")),
            "local");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(local_attributes, "local"), "value")),
            "local");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(local_attributes, "scope"), "value")),
            "local");

        // Verify global scope still has its own attributes
        sentry_value_t global_attributes = global_scope->attributes;
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(global_attributes, "all"), "value")),
            "global");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_value_get_by_key(global_attributes, "global"), "value")),
            "global");

        sentry__scope_free(local_scope);
    }

    // Test removing attributes from global scope
    sentry_remove_attribute("all");

    SENTRY_WITH_SCOPE (scope) {
        sentry_value_t attributes = scope->attributes;
        TEST_CHECK(
            sentry_value_is_null(sentry_value_get_by_key(attributes, "all")));
        // Other attributes should still exist
        TEST_CHECK(!sentry_value_is_null(
            sentry_value_get_by_key(attributes, "global")));
        TEST_CHECK(!sentry_value_is_null(
            sentry_value_get_by_key(attributes, "scope")));
    }

    // Test _n variants with local scope
    SENTRY_WITH_SCOPE (global_scope) {
        sentry_scope_t *local_scope = sentry_local_scope_new();
        sentry__scope_set_attribute_n(local_scope, "test_key", 8,
            sentry_value_new_attribute(sentry_value_new_int32(100), "percent"));

        sentry_value_t local_attributes = local_scope->attributes;
        sentry_value_t attr
            = sentry_value_get_by_key(local_attributes, "test_key");

        TEST_CHECK(!sentry_value_is_null(attr));
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(attr, "type")),
            "integer");
        TEST_CHECK(sentry_value_as_int32(sentry_value_get_by_key(attr, "value"))
            == 100);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(attr, "unit")),
            "percent");

        // Remove using _n variant
        sentry__scope_remove_attribute_n(local_scope, "test_key", 8);
        TEST_CHECK(sentry_value_is_null(
            sentry_value_get_by_key(local_attributes, "test_key")));

        sentry__scope_free(local_scope);
    }

    // Test that invalid attributes are not set on local scope
    SENTRY_WITH_SCOPE (global_scope) {
        sentry_scope_t *local_scope = sentry_local_scope_new();

        // Try to set invalid attribute (missing 'value')
        sentry_value_t invalid_attr = sentry_value_new_object();
        sentry_value_set_by_key(
            invalid_attr, "type", sentry_value_new_string("string"));
        sentry__scope_set_attribute(local_scope, "invalid", invalid_attr);

        sentry_value_t local_attributes = local_scope->attributes;
        TEST_CHECK(sentry_value_is_null(
            sentry_value_get_by_key(local_attributes, "invalid")));
        sentry_value_decref(invalid_attr);

        sentry__scope_free(local_scope);
    }

    sentry_close();
}

typedef struct {
    sentry_value_t release;
    sentry_value_t environment;
    sentry_value_t transaction;
    sentry_value_t fingerprint;
    sentry_level_t level;
    sentry_value_t user;
    sentry_value_t breadcrumbs;
    sentry_value_t tags;
    sentry_value_t extras;
    sentry_value_t contexts;
    sentry_value_t attachments;
    bool was_called;
} test_observer_data_t;

static void
observe_set_release(void *data, const char *release, size_t release_len)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    d->release = sentry_value_new_string_n(release, release_len);
    d->was_called = true;
}

static void
observe_set_environment(
    void *data, const char *environment, size_t environment_len)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    d->environment = sentry_value_new_string_n(environment, environment_len);
    d->was_called = true;
}

static void
observe_set_transaction(
    void *data, const char *transaction, size_t transaction_len)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    d->transaction = sentry_value_new_string_n(transaction, transaction_len);
    d->was_called = true;
}

static void
observe_set_fingerprint(void *data, sentry_value_t fingerprint)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (!sentry_value_is_null(d->fingerprint)) {
        sentry_value_decref(d->fingerprint);
    }
    sentry_value_incref(fingerprint);
    d->fingerprint = fingerprint;
    d->was_called = true;
}

static void
observe_set_level(void *data, sentry_level_t level)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    d->level = level;
    d->was_called = true;
}

static void
observe_add_attachment(void *data, sentry_attachment_t *attachment)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->attachments)) {
        d->attachments = sentry_value_new_list();
    }
    sentry_value_t obj = sentry_value_new_object();
    const char *filename = sentry__attachment_get_filename(attachment);
    if (filename) {
        sentry_value_set_by_key(
            obj, "filename", sentry_value_new_string(filename));
    }
    if (attachment->buf) {
        sentry_value_set_by_key(obj, "buf",
            sentry_value_new_string_n(attachment->buf, attachment->buf_len));
    }
    sentry_value_append(d->attachments, obj);
    d->was_called = true;
}

static void
observe_remove_attachment(void *data, sentry_attachment_t *attachment)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->attachments)) {
        d->attachments = sentry_value_new_list();
    }
    sentry_value_t obj = sentry_value_new_object();
    const char *filename = sentry__attachment_get_filename(attachment);
    if (filename) {
        sentry_value_set_by_key(
            obj, "filename", sentry_value_new_string(filename));
    }
    sentry_value_set_by_key(obj, "removed", sentry_value_new_bool(true));
    sentry_value_append(d->attachments, obj);
    d->was_called = true;
}

static void
observe_set_user(void *data, sentry_value_t user)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    d->user = user;
    d->was_called = true;
}

static void
observe_add_breadcrumb(void *data, sentry_value_t breadcrumb)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->breadcrumbs)) {
        d->breadcrumbs = sentry_value_new_list();
    }
    sentry_value_incref(breadcrumb);
    sentry_value_append(d->breadcrumbs, breadcrumb);
    d->was_called = true;
}

static void
observe_set_tag(void *data, const char *key, size_t key_len, const char *value,
    size_t value_len)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->tags)) {
        d->tags = sentry_value_new_object();
    }
    sentry_value_set_by_key_n(
        d->tags, key, key_len, sentry_value_new_string_n(value, value_len));
    d->was_called = true;
}

static void
observe_remove_tag(void *data, const char *key, size_t key_len)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->tags)) {
        d->tags = sentry_value_new_object();
    }
    sentry_value_set_by_key_n(
        d->tags, key, key_len, sentry_value_new_string("(removed)"));
    d->was_called = true;
}

static void
observe_set_extra(
    void *data, const char *key, size_t key_len, sentry_value_t value)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->extras)) {
        d->extras = sentry_value_new_object();
    }
    sentry_value_incref(value);
    sentry_value_set_by_key_n(d->extras, key, key_len, value);
    d->was_called = true;
}

static void
observe_remove_extra(void *data, const char *key, size_t key_len)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->extras)) {
        d->extras = sentry_value_new_object();
    }
    sentry_value_set_by_key_n(
        d->extras, key, key_len, sentry_value_new_string("(removed)"));
    d->was_called = true;
}

static void
observe_set_context(
    void *data, const char *key, size_t key_len, sentry_value_t value)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->contexts)) {
        d->contexts = sentry_value_new_object();
    }
    sentry_value_incref(value);
    sentry_value_set_by_key_n(d->contexts, key, key_len, value);
    d->was_called = true;
}

static void
observe_remove_context(void *data, const char *key, size_t key_len)
{
    test_observer_data_t *d = (test_observer_data_t *)data;
    if (sentry_value_is_null(d->contexts)) {
        d->contexts = sentry_value_new_object();
    }
    sentry_value_set_by_key_n(
        d->contexts, key, key_len, sentry_value_new_string("(removed)"));
    d->was_called = true;
}

SENTRY_TEST(scope_observer_null)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { .tags = sentry_value_new_null() };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_tag = observe_set_tag;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_set_tag("my-tag", "my-value");
    TEST_CHECK(d.was_called);

    d.was_called = false;
    sentry_remove_tag("my-tag");
    TEST_CHECK(!d.was_called);

    sentry_value_decref(d.tags);

    sentry_close();
}

SENTRY_TEST(scope_observer_multiple)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d1 = { .tags = sentry_value_new_null() };
    test_observer_data_t d2 = { .tags = sentry_value_new_null() };
    sentry_scope_observer_t *observer1 = sentry__scope_observer_new();
    observer1->data = &d1;
    observer1->set_tag = observe_set_tag;

    sentry_scope_observer_t *observer2 = sentry__scope_observer_new();
    observer2->data = &d2;
    observer2->set_tag = observe_set_tag;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer1);
        sentry__scope_add_observer(scope, observer2);
    }

    sentry_set_tag("multi", "test");
    TEST_CHECK(d1.was_called);
    TEST_CHECK(d2.was_called);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(d1.tags, "multi")),
        "test");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(d2.tags, "multi")),
        "test");

    sentry_value_decref(d1.tags);
    sentry_value_decref(d2.tags);
    sentry_close();
}

SENTRY_TEST(scope_observer_release)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { 0 };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_release = observe_set_release;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_set_release("my-release");
    TEST_CHECK(d.was_called);
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(d.release), "my-release");

    sentry_value_decref(d.release);
    sentry_close();
}

SENTRY_TEST(scope_observer_environment)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { 0 };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_environment = observe_set_environment;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_set_environment("my-env");
    TEST_CHECK(d.was_called);
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(d.environment), "my-env");

    sentry_value_decref(d.environment);
    sentry_close();
}

SENTRY_TEST(scope_observer_transaction)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { 0 };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_transaction = observe_set_transaction;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_set_transaction("my-transaction");
    TEST_CHECK(d.was_called);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(d.transaction), "my-transaction");

    sentry_value_decref(d.transaction);
    sentry_close();
}

SENTRY_TEST(scope_observer_fingerprint)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { 0 };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_fingerprint = observe_set_fingerprint;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_set_fingerprint("my-fingerprint", NULL);
    TEST_CHECK(d.was_called);
    TEST_CHECK(!sentry_value_is_null(d.fingerprint));
    TEST_CHECK_JSON_VALUE(d.fingerprint, "[\"my-fingerprint\"]");

    d.was_called = false;
    sentry_remove_fingerprint();
    TEST_CHECK(d.was_called);
    TEST_CHECK(sentry_value_is_null(d.fingerprint));

    sentry_close();
}

SENTRY_TEST(scope_observer_level)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { 0 };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_level = observe_set_level;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_set_level(SENTRY_LEVEL_WARNING);
    TEST_CHECK(d.was_called);
    TEST_CHECK(d.level == SENTRY_LEVEL_WARNING);

    sentry_close();
}

SENTRY_TEST(scope_observer_user)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { 0 };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_user = observe_set_user;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(user, "id", sentry_value_new_string("user123"));
    sentry_set_user(user);
    TEST_CHECK(d.was_called);
    TEST_CHECK(!sentry_value_is_null(d.user));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(d.user, "id")),
        "user123");

    d.was_called = false;
    sentry_remove_user();
    TEST_CHECK(d.was_called);
    TEST_CHECK(sentry_value_is_null(d.user));

    sentry_close();
}

SENTRY_TEST(scope_observer_breadcrumbs)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { .breadcrumbs = sentry_value_new_null() };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->add_breadcrumb = observe_add_breadcrumb;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_add_breadcrumb(
        sentry_value_new_breadcrumb(NULL, "first breadcrumb"));
    TEST_CHECK(d.was_called);
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(d.breadcrumbs), 1);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(
            sentry_value_get_by_index(d.breadcrumbs, 0), "message")),
        "first breadcrumb");

    sentry_add_breadcrumb(
        sentry_value_new_breadcrumb("warning", "second breadcrumb"));
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(d.breadcrumbs), 2);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(
            sentry_value_get_by_index(d.breadcrumbs, 1), "message")),
        "second breadcrumb");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(
            sentry_value_get_by_index(d.breadcrumbs, 1), "type")),
        "warning");

    sentry_value_decref(d.breadcrumbs);
    sentry_close();
}

SENTRY_TEST(scope_observer_tags)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { .tags = sentry_value_new_null() };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_tag = observe_set_tag;
    observer->remove_tag = observe_remove_tag;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_set_tag("my-tag", "my-value");
    TEST_CHECK(d.was_called);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(d.tags, "my-tag")),
        "my-value");
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_length(sentry_value_get_by_key(d.tags, "my-tag")), 8);

    d.was_called = false;
    sentry_remove_tag("my-tag");
    TEST_CHECK(d.was_called);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(d.tags, "my-tag")),
        "(removed)");

    sentry_value_decref(d.tags);
    sentry_close();
}

SENTRY_TEST(scope_observer_extras)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { .extras = sentry_value_new_null() };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_extra = observe_set_extra;
    observer->remove_extra = observe_remove_extra;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_value_t val = sentry_value_new_string("extra-value");
    sentry_set_extra("my-extra", val);
    TEST_CHECK(d.was_called);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(d.extras, "my-extra")),
        "extra-value");

    d.was_called = false;
    sentry_remove_extra("my-extra");
    TEST_CHECK(d.was_called);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(d.extras, "my-extra")),
        "(removed)");

    sentry_value_decref(d.extras);
    sentry_close();
}

SENTRY_TEST(scope_observer_contexts)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { .contexts = sentry_value_new_null() };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->set_context = observe_set_context;
    observer->remove_context = observe_remove_context;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_value_t ctx = sentry_value_new_object();
    sentry_value_set_by_key(ctx, "type", sentry_value_new_string("device"));
    sentry_set_context("my-context", ctx);
    TEST_CHECK(d.was_called);
    sentry_value_t received = sentry_value_get_by_key(d.contexts, "my-context");
    TEST_CHECK(!sentry_value_is_null(received));
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(received, "type")),
        "device");

    d.was_called = false;
    sentry_remove_context("my-context");
    TEST_CHECK(d.was_called);
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                d.contexts, "my-context")),
        "(removed)");

    sentry_value_decref(d.contexts);
    sentry_close();
}

SENTRY_TEST(scope_observer_attachments)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    test_observer_data_t d = { .attachments = sentry_value_new_null() };
    sentry_scope_observer_t *observer = sentry__scope_observer_new();
    observer->data = &d;
    observer->add_attachment = observe_add_attachment;
    observer->remove_attachment = observe_remove_attachment;

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__scope_add_observer(scope, observer);
    }

    sentry_attachment_t *attachment = sentry_attach_bytes("buf", 3, "test.txt");
    TEST_CHECK(d.was_called);
    TEST_CHECK(attachment != NULL);
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(d.attachments), 1);
    sentry_value_t added = sentry_value_get_by_index(d.attachments, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(added, "buf")), "buf");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(added, "filename")),
        "test.txt");

    d.was_called = false;
    sentry_remove_attachment(attachment);
    TEST_CHECK(d.was_called);
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(d.attachments), 2);
    sentry_value_t removed = sentry_value_get_by_index(d.attachments, 1);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(removed, "filename")),
        "test.txt");
    TEST_CHECK(
        sentry_value_is_true(sentry_value_get_by_key(removed, "removed")));

    sentry_value_decref(d.attachments);
    sentry_close();
}
