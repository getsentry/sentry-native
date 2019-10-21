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

TEST_CASE("value refcounting", "[value]") {
    // we start out with a refcount of 1
    sentry::Value val = sentry::Value::new_object();
    REQUIRE(val.refcount() == 1);
    val.set_by_key("key1", sentry::Value::new_string("value1"));

    // if i make a second value the refcount increases
    sentry::Value val2(val);
    REQUIRE(val.refcount() == 2);
    REQUIRE(val2.refcount() == 2);

    // when i lower to the value_t type the refcount stays the same
    // but the specific instance lowered nulls out.
    sentry_value_t val_l = val2.lower();
    REQUIRE(val.refcount() == 2);
    REQUIRE(val2.is_null());

    // using the CABI to access a value bumps the refcount:
    sentry_value_t child_val_l = sentry_value_get_by_key(val_l, "key1");
    REQUIRE(!sentry_value_is_null(child_val_l));

    // refcount is 3: 1 for the value itself, 2 for the return value of
    // get_by_key and one because we call sentry::Value on it to get the
    // refcount which bumps it by one as well.
    REQUIRE(sentry::Value(child_val_l).refcount() == 3);

    // when I consume a value it inherits the refcount.
    val2 = sentry::Value::consume(val_l);
    REQUIRE(val.refcount() == 2);
    REQUIRE(val2.refcount() == 2);
}
