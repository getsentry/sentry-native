#include "sentry.h"
#include "sentry_scope.h"
#include "sentry_testsupport.h"

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

    // scope: {"both":"scope","scope":"scope"}
    sentry_set_context("both", sentry_value_new_string("scope"));
    sentry_set_context("scope", sentry_value_new_string("scope"));

    SENTRY_WITH_SCOPE (scope) {
        // event: {"both":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t contexts = sentry_value_new_object();
            sentry_value_set_by_key(
                contexts, "both", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                contexts, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "contexts", contexts);
        }

        // event <- scope: {"both":"event","event":"event","scope":"scope"}
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_CONTEXT_EQUAL(event, "both", "event");
        TEST_CHECK_CONTEXT_EQUAL(event, "event", "event");
        TEST_CHECK_CONTEXT_EQUAL(event, "scope", "scope");

        sentry_value_decref(event);
    }

#undef TEST_CHECK_CONTEXT_EQUAL

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

    // scope: {"both":"scope","scope":"scope"}
    sentry_set_extra("both", sentry_value_new_string("scope"));
    sentry_set_extra("scope", sentry_value_new_string("scope"));

    SENTRY_WITH_SCOPE (scope) {
        // event: {"both":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t extra = sentry_value_new_object();
            sentry_value_set_by_key(
                extra, "both", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                extra, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "extra", extra);
        }

        // event <- scope: {"both":"event","event":"event","scope":"scope"}
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_EXTRA_EQUAL(event, "both", "event");
        TEST_CHECK_EXTRA_EQUAL(event, "event", "event");
        TEST_CHECK_EXTRA_EQUAL(event, "scope", "scope");

        sentry_value_decref(event);
    }

#undef TEST_CHECK_EXTRA_EQUAL

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

    // scope: {"both":"scope","scope":"scope"}
    sentry_set_tag("both", "scope");
    sentry_set_tag("scope", "scope");

    SENTRY_WITH_SCOPE (scope) {
        // event: {"both":"event","event":"event"}
        sentry_value_t event = sentry_value_new_object();
        {
            sentry_value_t tags = sentry_value_new_object();
            sentry_value_set_by_key(
                tags, "both", sentry_value_new_string("event"));
            sentry_value_set_by_key(
                tags, "event", sentry_value_new_string("event"));
            sentry_value_set_by_key(event, "tags", tags);
        }

        // event <- scope: {"both":"event","event":"event","scope":"scope"}
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_NONE);
        TEST_CHECK_TAG_EQUAL(event, "both", "event");
        TEST_CHECK_TAG_EQUAL(event, "event", "event");
        TEST_CHECK_TAG_EQUAL(event, "scope", "scope");

        sentry_value_decref(event);
    }

#undef TEST_CHECK_TAG_EQUAL

    sentry_close();
}
