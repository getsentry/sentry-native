#include <sentry.h>
#include <string>
#include <vendor/catch.hpp>

TEST_CASE("simple uuid api", "[uuid]") {
    sentry_uuid_t uuid =
        sentry_uuid_from_string("f391fdc0-bb27-43b1-8c0c-183bc217d42b");
    REQUIRE(!sentry_uuid_is_nil(&uuid));
    std::string uuid_string;
    uuid_string.resize(36);
    sentry_uuid_as_string(&uuid, &uuid_string[0]);
    REQUIRE(uuid_string == "f391fdc0-bb27-43b1-8c0c-183bc217d42b");

    uuid = sentry_uuid_from_bytes(
        "\xf3\x91\xfd\xc0\xbb'C\xb1\x8c\x0c\x18;\xc2\x17\xd4+");
    sentry_uuid_as_string(&uuid, &uuid_string[0]);
    REQUIRE(uuid_string == "f391fdc0-bb27-43b1-8c0c-183bc217d42b");
}

TEST_CASE("simple uuid generation", "[uuid]") {
    for (int i = 0; i < 50; i++) {
        sentry_uuid_t uuid = sentry_uuid_new_v4();
        REQUIRE(!sentry_uuid_is_nil(&uuid));
        char bytes[16];
        sentry_uuid_as_bytes(&uuid, bytes);
        REQUIRE(bytes[6] >> 4 == 4);
    }
}
