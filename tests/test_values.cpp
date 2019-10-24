#include <sentry.h>
#include <cmath>
#include <vendor/catch.hpp>

#define REQUIRE_VALUE_JSON(Val, ReferenceJson)       \
    do {                                             \
        char *json = sentry_value_to_json(Val);      \
        REQUIRE(json == std::string(ReferenceJson)); \
        sentry_string_free(json);                    \
    } while (0)
#define REQUIRE_VALUE_MSGPACK(Val, ReferenceMsgPack)                          \
    do {                                                                      \
        size_t len;                                                           \
        char *msgpack = sentry_value_to_msgpack(Val, &len);                   \
        REQUIRE(std::string(msgpack, len) ==                                  \
                std::string(ReferenceMsgPack, sizeof(ReferenceMsgPack) - 1)); \
        sentry_string_free(msgpack);                                          \
    } while (0)

TEST_CASE("primitive null behavior", "[value]") {
    sentry_value_t val = sentry_value_new_null();
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_NULL);
    REQUIRE(sentry_value_is_null(val));
    REQUIRE(sentry_value_as_int32(val) == 0);
    REQUIRE(std::isnan(sentry_value_as_double(val)));
    REQUIRE(sentry_value_is_true(val) == false);
    REQUIRE_VALUE_JSON(val, "null");
    REQUIRE_VALUE_MSGPACK(val, "\xc0");
    sentry_value_decref(val);
}

TEST_CASE("primitive bool behavior", "[value]") {
    sentry_value_t val = sentry_value_new_bool(true);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    REQUIRE(sentry_value_as_int32(val) == 0);
    REQUIRE(std::isnan(sentry_value_as_double(val)));
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_JSON(val, "true");
    REQUIRE_VALUE_MSGPACK(val, "\xc3");
    sentry_value_decref(val);

    val = sentry_value_new_bool(false);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    REQUIRE(sentry_value_as_int32(val) == 0);
    REQUIRE(std::isnan(sentry_value_as_double(val)));
    REQUIRE(sentry_value_is_true(val) == false);
    REQUIRE_VALUE_JSON(val, "false");
    REQUIRE_VALUE_MSGPACK(val, "\xc2");
    sentry_value_decref(val);
}

TEST_CASE("primitive int32 behavior", "[value]") {
    sentry_value_t val = sentry_value_new_int32(42);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    REQUIRE(sentry_value_as_int32(val) == 42);
    REQUIRE(sentry_value_as_double(val) == 42.0);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_JSON(val, "42");
    REQUIRE_VALUE_MSGPACK(val, "*");
    sentry_value_decref(val);

    val = sentry_value_new_int32(-1);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    REQUIRE(sentry_value_as_int32(val) == -1);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_MSGPACK(val, "\xff");
    sentry_value_decref(val);
}

TEST_CASE("primitive double behavior", "[value]") {
    sentry_value_t val = sentry_value_new_double(42.05);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_DOUBLE);
    REQUIRE(sentry_value_as_double(val) == 42.05);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_JSON(val, "42.05");
    REQUIRE_VALUE_MSGPACK(val,
                          "\xcb@E\x06"
                          "fffff");
    sentry_value_decref(val);
}

TEST_CASE("primitive string behavior", "[value]") {
    sentry_value_t val = sentry_value_new_string("Hello World!\n\t\r\f");
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_STRING);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE(sentry_value_as_string(val) == std::string("Hello World!\n\t\r\f"));
    REQUIRE_VALUE_JSON(val, "\"Hello World!\\n\\t\\r\\f\"");
    REQUIRE_VALUE_MSGPACK(val, "\xb0Hello World!\n\t\r\x0c");
    sentry_value_decref(val);
}

TEST_CASE("list value behavior", "[value]") {
    sentry_value_t val = sentry_value_new_list();
    for (size_t i = 0; i < 10; i++) {
        sentry_value_append(val, sentry_value_new_int32((int32_t)i));
    }
    for (size_t i = 0; i < 20; i++) {
        sentry_value_t child = sentry_value_get_by_index(val, i);
        if (i < 10) {
            REQUIRE(sentry_value_as_int32(child) == (int32_t)i);
        } else {
            REQUIRE(sentry_value_is_null(child));
        }
    }
    REQUIRE(sentry_value_get_length(val) == 10);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_LIST);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_JSON(val, "[0,1,2,3,4,5,6,7,8,9]");
    REQUIRE_VALUE_MSGPACK(val, "\x9a\x00\x01\x02\x03\x04\x05\x06\x07\x08\t");
    sentry_value_decref(val);

    val = sentry_value_new_list();
    REQUIRE(sentry_value_is_true(val) == false);
    REQUIRE_VALUE_JSON(val, "[]");
    REQUIRE_VALUE_MSGPACK(val, "\x90");
    sentry_value_decref(val);

    val = sentry_value_new_list();
    sentry_value_set_by_index(val, 5, sentry_value_new_int32(100));
    sentry_value_set_by_index(val, 2, sentry_value_new_int32(10));
    REQUIRE_VALUE_JSON(val, "[null,null,10,null,null,100]");
    sentry_value_remove_by_index(val, 2);
    REQUIRE_VALUE_JSON(val, "[null,null,null,null,100]");
    sentry_value_decref(val);
}

TEST_CASE("object value behavior", "[value]") {
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
            REQUIRE(sentry_value_as_int32(child) == (int32_t)i);
        } else {
            REQUIRE(sentry_value_is_null(child));
        }
    }
    REQUIRE(sentry_value_get_length(val) == 10);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_OBJECT);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_JSON(
        val,
        "{\"key0\":0,\"key1\":1,\"key2\":2,\"key3\":3,\"key4\":4,\"key5\":5,"
        "\"key6\":6,\"key7\":7,\"key8\":8,\"key9\":9}");
    REQUIRE_VALUE_MSGPACK(
        val,
        "\x8a\xa4key0\x00\xa4key1\x01\xa4key2\x02\xa4key3\x03\xa4key4\x04\xa4ke"
        "y5\x05\xa4key6\x06\xa4key7\x07\xa4key8\x08\xa4key9\t");
    sentry_value_decref(val);

    val = sentry_value_new_object();
    REQUIRE(sentry_value_is_true(val) == false);
    REQUIRE_VALUE_JSON(val, "{}");
    REQUIRE_VALUE_MSGPACK(val, "\x80");
    sentry_value_decref(val);
}
