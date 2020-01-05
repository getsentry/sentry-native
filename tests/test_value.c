#include "../src/sentry_value.h"
#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(value_null)
{
    sentry_value_t val = sentry_value_new_null();
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_NULL);
    TEST_ASSERT(sentry_value_is_null(val));
    TEST_ASSERT(sentry_value_as_int32(val) == 0);
    TEST_ASSERT(!sentry_value_is_true(val));
    ASSERT_JSON_VALUE(val, "null");
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    TEST_ASSERT(sentry_value_is_frozen(val));
}

SENTRY_TEST(value_bool)
{
    sentry_value_t val = sentry_value_new_bool(true);
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    TEST_ASSERT(sentry_value_as_int32(val) == 0);
    TEST_ASSERT(sentry_value_is_true(val));
    ASSERT_JSON_VALUE(val, "true");
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    TEST_ASSERT(sentry_value_is_frozen(val));

    val = sentry_value_new_bool(false);
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    TEST_ASSERT(sentry_value_as_int32(val) == 0);
    TEST_ASSERT(!sentry_value_is_true(val));
    ASSERT_JSON_VALUE(val, "false");
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    TEST_ASSERT(sentry_value_is_frozen(val));
}

SENTRY_TEST(value_int32)
{
    sentry_value_t val = sentry_value_new_int32(42);
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    TEST_ASSERT(sentry_value_as_int32(val) == 42);
    TEST_ASSERT(sentry_value_as_double(val) == 42.0);
    TEST_ASSERT(sentry_value_is_true(val));
    ASSERT_JSON_VALUE(val, "42");
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    TEST_ASSERT(sentry_value_refcount(val) == 1);

    for (int32_t i = -255; i < 255; i++) {
        val = sentry_value_new_int32(i);
        ASSERT_INT_EQUAL((int)i, (int)sentry_value_as_int32(val));
        TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    }

    val = sentry_value_new_int32(-1);
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    TEST_ASSERT(sentry_value_as_int32(val) == -1);
    TEST_ASSERT(sentry_value_is_true(val) == true);
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    TEST_ASSERT(sentry_value_is_frozen(val));
}

SENTRY_TEST(value_double)
{
    sentry_value_t val = sentry_value_new_double(42.05);
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_DOUBLE);
    TEST_ASSERT(sentry_value_as_double(val) == 42.05);
    TEST_ASSERT(sentry_value_is_true(val));
    ASSERT_JSON_VALUE(val, "42.05");
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    TEST_ASSERT(sentry_value_is_frozen(val));
}

SENTRY_TEST(value_string)
{
    sentry_value_t val = sentry_value_new_string("Hello World!\n\t\r\f");
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_STRING);
    TEST_ASSERT(sentry_value_is_true(val) == true);
    ASSERT_STRING_EQUAL(sentry_value_as_string(val), "Hello World!\n\t\r\f");
    ASSERT_JSON_VALUE(val, "\"Hello World!\\n\\t\\r\\f\"");
    TEST_ASSERT(sentry_value_refcount(val) == 1);
    TEST_ASSERT(sentry_value_is_frozen(val));
    sentry_value_decref(val);
}

SENTRY_TEST(value_list)
{
    sentry_value_t val = sentry_value_new_list();
    for (size_t i = 0; i < 10; i++) {
        TEST_ASSERT(
            !sentry_value_append(val, sentry_value_new_int32((int32_t)i)));
    }
    for (size_t i = 0; i < 20; i++) {
        sentry_value_t child = sentry_value_get_by_index(val, i);
        if (i < 10) {
            TEST_ASSERT(
                sentry_value_get_type(child) == SENTRY_VALUE_TYPE_INT32);
            TEST_ASSERT(sentry_value_as_int32(child) == (int32_t)i);
        } else {
            TEST_ASSERT(sentry_value_is_null(child));
        }
    }
    TEST_ASSERT(sentry_value_get_length(val) == 10);
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_LIST);
    TEST_ASSERT(sentry_value_is_true(val) == true);
    ASSERT_JSON_VALUE(val, "[0,1,2,3,4,5,6,7,8,9]");
    sentry_value_decref(val);

    val = sentry_value_new_list();
    TEST_ASSERT(sentry_value_is_true(val) == false);
    ASSERT_JSON_VALUE(val, "[]");
    sentry_value_decref(val);

    val = sentry_value_new_list();
    sentry_value_set_by_index(val, 5, sentry_value_new_int32(100));
    sentry_value_set_by_index(val, 2, sentry_value_new_int32(10));
    ASSERT_JSON_VALUE(val, "[null,null,10,null,null,100]");
    sentry_value_remove_by_index(val, 2);
    ASSERT_JSON_VALUE(val, "[null,null,null,null,100]");
    TEST_ASSERT(!sentry_value_is_frozen(val));
    sentry_value_freeze(val);
    TEST_ASSERT(sentry_value_is_frozen(val));
    sentry_value_decref(val);

    val = sentry_value_new_list();
    for (uint32_t i = 1; i <= 10; i++) {
        sentry_value_append(val, sentry_value_new_int32(i));
    }
    sentry__value_append_bounded(val, sentry_value_new_int32(1010), 5);
#define ASSERT_IDX(Idx, Val)                                                   \
    ASSERT_INT_EQUAL(                                                          \
        sentry_value_as_int32(sentry_value_get_by_index(val, Idx)), Val)
    ASSERT_IDX(0, 7);
    ASSERT_IDX(1, 8);
    ASSERT_IDX(2, 9);
    ASSERT_IDX(3, 10);
    ASSERT_IDX(4, 1010);
    sentry_value_decref(val);
}

SENTRY_TEST(value_object)
{
    sentry_value_t val = sentry_value_new_object();
    for (size_t i = 0; i < 10; i++) {
        char key[100];
        sprintf(key, "key%d", (int)i);
        sentry_value_set_by_key(val, key, sentry_value_new_int32(i));
    }
    for (size_t i = 0; i < 20; i++) {
        char key[100];
        sprintf(key, "key%d", (int)i);
        sentry_value_t child = sentry_value_get_by_key(val, key);
        if (i < 10) {
            TEST_ASSERT(sentry_value_as_int32(child) == (int32_t)i);
        } else {
            TEST_ASSERT(sentry_value_is_null(child));
        }
    }
    TEST_ASSERT(sentry_value_get_length(val) == 10);
    TEST_ASSERT(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_OBJECT);
    TEST_ASSERT(sentry_value_is_true(val) == true);
    ASSERT_JSON_VALUE(val,
        "{\"key0\":0,\"key1\":1,\"key2\":2,\"key3\":3,\"key4\":4,\"key5\":5,"
        "\"key6\":6,\"key7\":7,\"key8\":8,\"key9\":9}");
    sentry_value_decref(val);

    val = sentry_value_new_object();
    TEST_ASSERT(sentry_value_is_true(val) == false);
    ASSERT_JSON_VALUE(val, "{}");
    TEST_ASSERT(!sentry_value_is_frozen(val));
    sentry_value_freeze(val);
    TEST_ASSERT(sentry_value_is_frozen(val));
    sentry_value_decref(val);
}

SENTRY_TEST(value_freezing)
{
    sentry_value_t val = sentry_value_new_list();
    sentry_value_t inner = sentry_value_new_object();
    sentry_value_append(val, inner);
    TEST_ASSERT(!sentry_value_is_frozen(val));
    TEST_ASSERT(!sentry_value_is_frozen(inner));
    sentry_value_freeze(val);
    TEST_ASSERT(sentry_value_is_frozen(val));
    TEST_ASSERT(sentry_value_is_frozen(inner));

    ASSERT_INT_EQUAL(sentry_value_append(val, sentry_value_new_bool(1)), 1);
    ASSERT_INT_EQUAL(sentry_value_get_length(val), 1);

    ASSERT_INT_EQUAL(
        sentry_value_set_by_key(inner, "foo", sentry_value_new_bool(1)), 1);
    ASSERT_INT_EQUAL(sentry_value_get_length(inner), 0);

    sentry_value_decref(val);
}