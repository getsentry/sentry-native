#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(uuid_api)
{
    sentry_uuid_t uuid
        = sentry_uuid_from_string("f391fdc0-bb27-43b1-8c0c-183bc217d42b");
    assert_false(sentry_uuid_is_nil(&uuid));
    char buf[37];
    sentry_uuid_as_string(&uuid, buf);
    assert_string_equal(buf, "f391fdc0-bb27-43b1-8c0c-183bc217d42b");

    uuid = sentry_uuid_from_bytes(
        "\xf3\x91\xfd\xc0\xbb'C\xb1\x8c\x0c\x18;\xc2\x17\xd4+");
    sentry_uuid_as_string(&uuid, buf);
    assert_string_equal(buf, "f391fdc0-bb27-43b1-8c0c-183bc217d42b");
}

SENTRY_TEST(uuid_v4)
{
    for (int i = 0; i < 50; i++) {
        sentry_uuid_t uuid = sentry_uuid_new_v4();
        assert_false(sentry_uuid_is_nil(&uuid));
        char bytes[16];
        sentry_uuid_as_bytes(&uuid, bytes);
        assert_true(bytes[6] >> 4 == 4);
    }
}