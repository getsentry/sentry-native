#include "sentry.h"
#include "sentry_scope.h"
#include "sentry_testsupport.h"

SENTRY_TEST(scope_contexts)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_set_context(
        "global-context", sentry_value_new_string("global-value"));

    sentry_scope_t *local_scope = sentry__scope_push();

    SENTRY_WITH_SCOPE (cloned_scope) {
        TEST_CHECK(cloned_scope == local_scope);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_context(
                                    cloned_scope, "global-context")),
            "global-value");
    }

    sentry_value_t local_contexts = sentry_value_new_object();
    sentry_value_set_by_key(local_contexts, "local-context",
        sentry_value_new_string("local-value"));
    sentry_scope_set_contexts(local_scope, local_contexts);

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_context(
                                    modified_scope, "local-context")),
            "local-value");
    }

    sentry__scope_pop();

    SENTRY_WITH_SCOPE (global_scope) {
        TEST_CHECK(global_scope != local_scope);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_context(
                                    global_scope, "global-context")),
            "global-value");
    }

    sentry_close();
}

SENTRY_TEST(scope_extras)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_set_extra("global-extra", sentry_value_new_string("global-value"));

    sentry_scope_t *local_scope = sentry__scope_push();

    SENTRY_WITH_SCOPE (cloned_scope) {
        TEST_CHECK(cloned_scope == local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_value_get_length(sentry_scope_get_extras(cloned_scope)), 1);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_extra(
                                    cloned_scope, "global-extra")),
            "global-value");
    }

    sentry_value_t local_extras = sentry_value_new_object();
    sentry_value_set_by_key(
        local_extras, "local-extra", sentry_value_new_string("local-value"));
    sentry_scope_set_extras(local_scope, local_extras);

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_value_get_length(sentry_scope_get_extras(modified_scope)),
            1);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_extra(
                                    modified_scope, "local-extra")),
            "local-value");
    }

    sentry__scope_pop();

    SENTRY_WITH_SCOPE (global_scope) {
        TEST_CHECK(global_scope != local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_value_get_length(sentry_scope_get_extras(global_scope)), 1);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_extra(
                                    global_scope, "global-extra")),
            "global-value");
    }

    sentry_close();
}

SENTRY_TEST(scope_fingerprint)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_set_fingerprint("global", "fingerprint", NULL);

    sentry_scope_t *local_scope = sentry__scope_push();

    SENTRY_WITH_SCOPE (cloned_scope) {
        TEST_CHECK(cloned_scope == local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_value_get_length(sentry_scope_get_fingerprint(cloned_scope)),
            2);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_index(
                sentry_scope_get_fingerprint(cloned_scope), 0)),
            "global");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_index(
                sentry_scope_get_fingerprint(cloned_scope), 1)),
            "fingerprint");
    }

    sentry_scope_set_fingerprint(local_scope, "local", "fingerprint", NULL);

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK_INT_EQUAL(sentry_value_get_length(
                                 sentry_scope_get_fingerprint(modified_scope)),
            2);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_index(
                sentry_scope_get_fingerprint(modified_scope), 0)),
            "local");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_index(
                sentry_scope_get_fingerprint(modified_scope), 1)),
            "fingerprint");
    }

    sentry__scope_pop();

    SENTRY_WITH_SCOPE (global_scope) {
        TEST_CHECK(global_scope != local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_value_get_length(sentry_scope_get_fingerprint(global_scope)),
            2);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_index(
                sentry_scope_get_fingerprint(global_scope), 0)),
            "global");
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_index(
                sentry_scope_get_fingerprint(global_scope), 1)),
            "fingerprint");
    }

    sentry_close();
}

SENTRY_TEST(scope_level)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_set_level(SENTRY_LEVEL_DEBUG);

    sentry_scope_t *local_scope = sentry__scope_push();

    SENTRY_WITH_SCOPE (cloned_scope) {
        TEST_CHECK(cloned_scope == local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_scope_get_level(cloned_scope), SENTRY_LEVEL_DEBUG);
    }

    sentry_scope_set_level(local_scope, SENTRY_LEVEL_ERROR);

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_scope_get_level(modified_scope), SENTRY_LEVEL_ERROR);
    }

    sentry__scope_pop();

    SENTRY_WITH_SCOPE (global_scope) {
        TEST_CHECK(global_scope != local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_scope_get_level(global_scope), SENTRY_LEVEL_DEBUG);
    }

    sentry_close();
}

SENTRY_TEST(scope_tags)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_set_tag("global-tag", "global-value");

    sentry_scope_t *local_scope = sentry__scope_push();

    SENTRY_WITH_SCOPE (cloned_scope) {
        TEST_CHECK(cloned_scope == local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_value_get_length(sentry_scope_get_tags(cloned_scope)), 1);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_scope_get_tags(cloned_scope), "global-tag")),
            "global-value");
    }

    sentry_value_t local_tags = sentry_value_new_object();
    sentry_value_set_by_key(
        local_tags, "local-tag", sentry_value_new_string("local-value"));
    sentry_scope_set_tags(local_scope, local_tags);

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_value_get_length(sentry_scope_get_tags(modified_scope)), 1);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_scope_get_tags(modified_scope), "local-tag")),
            "local-value");
    }

    sentry__scope_pop();

    SENTRY_WITH_SCOPE (global_scope) {
        TEST_CHECK(global_scope != local_scope);
        TEST_CHECK_INT_EQUAL(
            sentry_value_get_length(sentry_scope_get_tags(global_scope)), 1);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(
                sentry_scope_get_tags(global_scope), "global-tag")),
            "global-value");
    }

    sentry_close();
}

SENTRY_TEST(scope_transaction)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_set_transaction("global-transaction");

    sentry_scope_t *local_scope = sentry__scope_push();

    SENTRY_WITH_SCOPE (cloned_scope) {
        TEST_CHECK(cloned_scope == local_scope);
        TEST_CHECK_STRING_EQUAL(
            sentry_scope_get_transaction(cloned_scope), "global-transaction");
    }

    sentry_scope_set_transaction(local_scope, "local-transaction");

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK_STRING_EQUAL(
            sentry_scope_get_transaction(modified_scope), "local-transaction");
    }

    sentry__scope_pop();

    SENTRY_WITH_SCOPE (global_scope) {
        TEST_CHECK(global_scope != local_scope);
        TEST_CHECK_STRING_EQUAL(
            sentry_scope_get_transaction(global_scope), "global-transaction");
    }

    sentry_close();
}

SENTRY_TEST(scope_user)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_value_t global_user = sentry_value_new_object();
    sentry_value_set_by_key(
        global_user, "username", sentry_value_new_string("global_name"));
    sentry_set_user(global_user);

    sentry_scope_t *local_scope = sentry__scope_push();

    SENTRY_WITH_SCOPE (cloned_scope) {
        TEST_CHECK(cloned_scope == local_scope);
        TEST_CHECK_JSON_VALUE(sentry_scope_get_user(cloned_scope),
            "{\"username\":\"global_name\"}");
    }

    sentry_value_t local_user = sentry_value_new_object();
    sentry_value_set_by_key(
        local_user, "username", sentry_value_new_string("local_name"));
    sentry_scope_set_user(local_scope, local_user);

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK_JSON_VALUE(sentry_scope_get_user(modified_scope),
            "{\"username\":\"local_name\"}");
    }

    sentry_scope_remove_user(local_scope);

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK(sentry_value_is_null(sentry_scope_get_user(modified_scope)));
    }

    sentry__scope_pop();

    SENTRY_WITH_SCOPE (global_scope) {
        TEST_CHECK(global_scope != local_scope);
        TEST_CHECK_JSON_VALUE(sentry_scope_get_user(global_scope),
            "{\"username\":\"global_name\"}");
    }

    sentry_close();
}

SENTRY_TEST(scope_clear)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(
        user, "username", sentry_value_new_string("test-user"));
    sentry_set_user(user);

    sentry_set_tag("test-tag", "ttt");
    sentry_set_extra("test-extra", sentry_value_new_string("eee"));
    sentry_set_context("test-context", sentry_value_new_string("ccc"));
    sentry_set_transaction("test-transaction");
    sentry_set_fingerprint("test-fingerprint", NULL);

    sentry_scope_t *local_scope = sentry__scope_push();

    SENTRY_WITH_SCOPE (cloned_scope) {
        TEST_CHECK(cloned_scope == local_scope);
        TEST_CHECK_JSON_VALUE(sentry_scope_get_user(cloned_scope),
            "{\"username\":\"test-user\"}");
        TEST_CHECK_STRING_EQUAL(
            sentry_scope_get_tag(cloned_scope, "test-tag"), "ttt");
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_extra(
                                    cloned_scope, "test-extra")),
            "eee");
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_context(
                                    cloned_scope, "test-context")),
            "ccc");
        TEST_CHECK_STRING_EQUAL(
            sentry_scope_get_transaction(cloned_scope), "test-transaction");
        TEST_CHECK_JSON_VALUE(sentry_scope_get_fingerprint(cloned_scope),
            "[\"test-fingerprint\"]");
    }

    sentry_scope_clear(local_scope);

    SENTRY_WITH_SCOPE (modified_scope) {
        TEST_CHECK(modified_scope == local_scope);
        TEST_CHECK(sentry_value_is_null(sentry_scope_get_user(modified_scope)));
        TEST_CHECK(!sentry_scope_get_tag(modified_scope, "test-tag"));
        TEST_CHECK(sentry_value_is_null(
            sentry_scope_get_extra(modified_scope, "test-extra")));
        TEST_CHECK(sentry_value_is_null(
            sentry_scope_get_context(modified_scope, "test-context")));
        TEST_CHECK(!sentry_scope_get_transaction(modified_scope));
        TEST_CHECK(
            sentry_value_is_null(sentry_scope_get_fingerprint(modified_scope)));
    }

    sentry__scope_pop();

    SENTRY_WITH_SCOPE (global_scope) {
        TEST_CHECK(global_scope != local_scope);
        TEST_CHECK_JSON_VALUE(sentry_scope_get_user(global_scope),
            "{\"username\":\"test-user\"}");
        TEST_CHECK_STRING_EQUAL(
            sentry_scope_get_tag(global_scope, "test-tag"), "ttt");
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_extra(
                                    global_scope, "test-extra")),
            "eee");
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_scope_get_context(
                                    global_scope, "test-context")),
            "ccc");
        TEST_CHECK_STRING_EQUAL(
            sentry_scope_get_transaction(global_scope), "test-transaction");
        TEST_CHECK_JSON_VALUE(sentry_scope_get_fingerprint(global_scope),
            "[\"test-fingerprint\"]");
    }

    sentry_close();
}
