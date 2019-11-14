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
    sentry::Value list = sentry::Value::new_list();
    list.append(sentry::Value::new_string("1"));
    list.append(sentry::Value::new_string("2"));
    list.append(sentry::Value::new_string("3"));
    val.set_by_key("key2", list);

    // if i make a second value the refcount increases
    sentry::Value val2(val);
    REQUIRE(val.refcount() == 2);
    REQUIRE(val2.refcount() == 2);

    // when i lower to the value_t type the refcount stays the same
    // but the specific instance lowered nulls out.
    sentry_value_t val_l = val2.lower();
    REQUIRE(val.refcount() == 2);
    REQUIRE(val2.is_null());

    // using the CABI to access a value doesn't bump the refcount
    sentry_value_t child_val_l = sentry_value_get_by_key(val_l, "key1");
    sentry_value_t child_val2_l = sentry_value_get_by_key(val_l, "key1");
    REQUIRE(!sentry_value_is_null(child_val_l));
    REQUIRE(sentry::Value(child_val_l).refcount() == 2);
    REQUIRE(!sentry_value_is_null(child_val2_l));
    REQUIRE(sentry::Value(child_val2_l).refcount() == 2);

    // but if we look up owned it's bumped
    child_val_l = sentry_value_get_by_key_owned(val_l, "key1");
    REQUIRE(!sentry_value_is_null(child_val_l));
    REQUIRE(sentry::Value(child_val_l).refcount() == 3);
    sentry_value_decref(child_val_l);
    REQUIRE(sentry::Value(child_val_l).refcount() == 2);

    // same with lists.
    child_val_l = sentry_value_get_by_key(val_l, "key2");
    child_val2_l = sentry_value_get_by_index(child_val_l, 0);
    REQUIRE(sentry::Value(child_val2_l).refcount() == 2);
    REQUIRE(sentry::Value(child_val2_l).refcount() == 2);

    child_val2_l = sentry_value_get_by_index_owned(child_val_l, 0);
    REQUIRE(sentry::Value(child_val2_l).refcount() == 3);
    sentry_value_decref(child_val2_l);

    // when I consume a value it inherits the refcount.
    val2 = sentry::Value::consume(val_l);
    REQUIRE(val.refcount() == 2);
    REQUIRE(val2.refcount() == 2);
}

TEST_CASE("value json writing", "[value]") {
    sentry::Value event = sentry::Value::new_object();
    sentry::Value stacktrace = sentry::Value::new_list();
    sentry::Value frame = sentry::Value::new_object();
    frame.set_by_key("instruction_addr", sentry::Value::new_addr(0));
    stacktrace.append(frame);
    stacktrace.append(frame);
    stacktrace.append(frame);
    event.set_by_key("stacktrace", stacktrace);
    event.set_by_key("extra_stuff", sentry::Value::new_int32(0));
    REQUIRE(
        event.to_json() ==
        std::string(
            "{\"extra_stuff\":0,\"stacktrace\":[{\"instruction_addr\":\"0x0\"},"
            "{\"instruction_addr\":\"0x0\"},{\"instruction_addr\":\"0x0\"}]}"));
}

TEST_CASE("value freezing", "[value]") {
    sentry::Value int_val = sentry::Value::new_int32(42);
    REQUIRE(int_val.is_frozen() == true);

    sentry::Value list_val = sentry::Value::new_list();
    REQUIRE(list_val.is_frozen() == false);
    list_val.append(sentry::Value::new_object());
    list_val.append(sentry::Value::new_int32(0));
    list_val.append(sentry::Value::new_int32(1));
    list_val.append(sentry::Value::new_int32(2));
    REQUIRE(list_val.get_by_index(0).is_frozen() == false);
    REQUIRE(list_val.is_frozen() == false);
    list_val.freeze();
    REQUIRE(list_val.is_frozen() == true);
    REQUIRE(list_val.get_by_index(0).is_frozen() == true);

    sentry::Value list_clone_val = list_val.clone();
    REQUIRE(list_clone_val.is_frozen() == false);
    REQUIRE(list_val.get_by_index(0).is_frozen() == true);

    sentry::Value string_val = sentry::Value::new_string("hello");
    REQUIRE(string_val.is_frozen() == true);
}
