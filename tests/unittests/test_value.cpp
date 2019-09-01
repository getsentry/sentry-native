#include <string>
#include <value.hpp>
#include <vendor/catch.hpp>

TEST_CASE("value roundtips", "[value]") {
    sentry::Value intval = sentry::Value::new_int32(42);
    sentry_value_t intval_v = intval.lower();
    REQUIRE(intval.is_null());
    sentry::Value intval_rv(intval_v);
    REQUIRE(intval_rv.as_int32() == 42);
}

TEST_CASE("value from addr", "[value]") {
    sentry::Value val = sentry::Value::new_addr(0);
    REQUIRE(val.as_cstr() == std::string("0x0"));
    val = sentry::Value::new_addr(42);
    REQUIRE(val.as_cstr() == std::string("0x2a"));
    val = sentry::Value::new_addr(0xffffffffff);
    REQUIRE(val.as_cstr() == std::string("0xffffffffff"));
}

TEST_CASE("value from uuid", "[value]") {
    sentry_uuid_t uuid =
        sentry_uuid_from_string("f391fdc0-bb27-43b1-8c0c-183bc217d42b");
    sentry::Value val = sentry::Value::new_uuid(&uuid);
    REQUIRE(val.as_cstr() ==
            std::string("f391fdc0-bb27-43b1-8c0c-183bc217d42b"));
}

TEST_CASE("value from hexstring", "[value]") {
    sentry_uuid_t uuid =
        sentry_uuid_from_string("f391fdc0-bb27-43b1-8c0c-183bc217d42b");
    char bytes[16];
    sentry_uuid_as_bytes(&uuid, bytes);
    sentry::Value val = sentry::Value::new_hexstring(bytes, 16);
    REQUIRE(val.as_cstr() == std::string("f391fdc0bb2743b18c0c183bc217d42b"));
}
