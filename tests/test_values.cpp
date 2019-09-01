#include <sentry.h>
#include <cmath>
#include <vendor/catch.hpp>

#define REQUIRE_VALUE_JSON(Val, ReferenceJson)       \
    do {                                             \
        char *json = sentry_value_to_json(Val);      \
        REQUIRE(json == std::string(ReferenceJson)); \
        sentry_string_free(json);                    \
    } while (0)

TEST_CASE("primitive null behavior", "[value]") {
    sentry_value_t val = sentry_value_new_null();
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_NULL);
    REQUIRE(sentry_value_is_null(val));
    REQUIRE(sentry_value_as_int32(val) == 0);
    REQUIRE(std::isnan(sentry_value_as_double(val)));
    REQUIRE(sentry_value_is_true(val) == false);
    REQUIRE_VALUE_JSON(val, "null");
    sentry_value_free(val);
}

TEST_CASE("primitive bool behavior", "[value]") {
    sentry_value_t val = sentry_value_new_bool(true);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    REQUIRE(sentry_value_as_int32(val) == 0);
    REQUIRE(std::isnan(sentry_value_as_double(val)));
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_JSON(val, "true");
    sentry_value_free(val);

    val = sentry_value_new_bool(false);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_BOOL);
    REQUIRE(sentry_value_as_int32(val) == 0);
    REQUIRE(std::isnan(sentry_value_as_double(val)));
    REQUIRE(sentry_value_is_true(val) == false);
    REQUIRE_VALUE_JSON(val, "false");
    sentry_value_free(val);
}

TEST_CASE("primitive int32 behavior", "[value]") {
    sentry_value_t val = sentry_value_new_int32(42);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    REQUIRE(sentry_value_as_int32(val) == 42);
    REQUIRE(sentry_value_as_double(val) == 42.0);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_JSON(val, "42");
    sentry_value_free(val);

    val = sentry_value_new_int32((int32_t)(uint64_t)-1);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_INT32);
    REQUIRE(sentry_value_as_int32(val) == (int32_t)(uint64_t)-1);
    REQUIRE(sentry_value_is_true(val) == true);
    sentry_value_free(val);
}

TEST_CASE("primitive double behavior", "[value]") {
    sentry_value_t val = sentry_value_new_double(42.05);
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_DOUBLE);
    REQUIRE(sentry_value_as_double(val) == 42.05);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE_VALUE_JSON(val, "42.05");
    sentry_value_free(val);
}

TEST_CASE("primitive string behavior", "[value]") {
    sentry_value_t val = sentry_value_new_string("Hello World!\n\t\r\f");
    REQUIRE(sentry_value_get_type(val) == SENTRY_VALUE_TYPE_STRING);
    REQUIRE(sentry_value_is_true(val) == true);
    REQUIRE(sentry_value_as_string(val) == std::string("Hello World!\n\t\r\f"));
    REQUIRE_VALUE_JSON(val, "\"Hello World!\\n\\t\\r\\f\"");
    sentry_value_free(val);
}
