#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(value_null)
{
    sentry_value_t val = sentry_value_new_null();
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_NULL);
    assert_true(sentry_value_is_null(val));
    assert_true(sentry_value_as_int32(val) == 0);
    assert_false(sentry_value_is_true(val));
    assert_json_value(val, "null");
    assert_true(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    assert_true(sentry_value_refcount(val) == 1);
}

SENTRY_TEST(value_bool)
{
    sentry_value_t val = sentry_value_new_bool(true);
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    assert_true(sentry_value_as_int32(val) == 0);
    assert_true(sentry_value_is_true(val));
    assert_json_value(val, "true");
    assert_true(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    assert_true(sentry_value_refcount(val) == 1);

    val = sentry_value_new_bool(false);
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    assert_true(sentry_value_as_int32(val) == 0);
    assert_false(sentry_value_is_true(val));
    assert_json_value(val, "false");
    assert_true(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    assert_true(sentry_value_refcount(val) == 1);
}

SENTRY_TEST(value_int32)
{
    sentry_value_t val = sentry_value_new_int32(42);
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    assert_true(sentry_value_as_int32(val) == 42);
    assert_true(sentry_value_as_double(val) == 42.0);
    assert_true(sentry_value_is_true(val));
    assert_json_value(val, "42");
    assert_true(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    assert_true(sentry_value_refcount(val) == 1);

    for (int32_t i = -255; i < 255; i++) {
        val = sentry_value_new_int32(i);
        assert_int_equal((int)i, (int)sentry_value_as_int32(val));
        assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    }

    val = sentry_value_new_int32(-1);
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    assert_true(sentry_value_as_int32(val) == -1);
    assert_true(sentry_value_is_true(val) == true);
    assert_true(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    assert_true(sentry_value_refcount(val) == 1);
}

SENTRY_TEST(value_double)
{
    sentry_value_t val = sentry_value_new_double(42.05);
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_DOUBLE);
    assert_true(sentry_value_as_double(val) == 42.05);
    assert_true(sentry_value_is_true(val));
    assert_json_value(val, "42.05");
    assert_true(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
    assert_true(sentry_value_refcount(val) == 1);
}

SENTRY_TEST(value_string)
{
    sentry_value_t val = sentry_value_new_string("Hello World!\n\t\r\f");
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_STRING);
    assert_true(sentry_value_is_true(val) == true);
    assert_string_equal(sentry_value_as_string(val), "Hello World!\n\t\r\f");
    assert_json_value(val, "\"Hello World!\\n\\t\\r\\f\"");
    assert_true(sentry_value_refcount(val) == 1);
    sentry_value_decref(val);
}

SENTRY_TEST(value_list)
{
    sentry_value_t val = sentry_value_new_list();
    for (size_t i = 0; i < 10; i++) {
        assert_true(
            !sentry_value_append(val, sentry_value_new_int32((int32_t)i)));
    }
    for (size_t i = 0; i < 20; i++) {
        sentry_value_t child = sentry_value_get_by_index(val, i);
        if (i < 10) {
            assert_true(
                sentry_value_get_type(child) == SENTRY_VALUE_TYPE_INT32);
            assert_true(sentry_value_as_int32(child) == (int32_t)i);
        } else {
            assert_true(sentry_value_is_null(child));
        }
    }
    assert_true(sentry_value_get_length(val) == 10);
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_LIST);
    assert_true(sentry_value_is_true(val) == true);
    assert_json_value(val, "[0,1,2,3,4,5,6,7,8,9]");
    sentry_value_decref(val);

    val = sentry_value_new_list();
    assert_true(sentry_value_is_true(val) == false);
    assert_json_value(val, "[]");
    sentry_value_decref(val);

    val = sentry_value_new_list();
    sentry_value_set_by_index(val, 5, sentry_value_new_int32(100));
    sentry_value_set_by_index(val, 2, sentry_value_new_int32(10));
    assert_json_value(val, "[null,null,10,null,null,100]");
    sentry_value_remove_by_index(val, 2);
    assert_json_value(val, "[null,null,null,null,100]");
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
            assert_true(sentry_value_as_int32(child) == (int32_t)i);
        } else {
            assert_true(sentry_value_is_null(child));
        }
    }
    assert_true(sentry_value_get_length(val) == 10);
    assert_true(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_OBJECT);
    assert_true(sentry_value_is_true(val) == true);
    assert_json_value(val,
        "{\"key0\":0,\"key1\":1,\"key2\":2,\"key3\":3,\"key4\":4,\"key5\":5,"
        "\"key6\":6,\"key7\":7,\"key8\":8,\"key9\":9}");
    sentry_value_decref(val);

    val = sentry_value_new_object();
    assert_true(sentry_value_is_true(val) == false);
    assert_json_value(val, "{}");
    sentry_value_decref(val);
}