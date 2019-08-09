#include "uuid.hpp"

#ifdef _WIN32
#else
sentry_uuid_t sentry_uuid_nil() {
    sentry_uuid_t rv;
    uuid_clear(rv.native_uuid);
    return rv;
}

sentry_uuid_t sentry_uuid_new_v4() {
    sentry_uuid_t rv;
    uuid_generate_random(rv.native_uuid);
    return rv;
}

sentry_uuid_t sentry_uuid_from_string(const char *str) {
    sentry_uuid_t rv;
    if (uuid_parse(str, rv.native_uuid) != 0) {
        uuid_clear(rv.native_uuid);
    }
    return rv;
}

int sentry_uuid_is_nil(const sentry_uuid_t *uuid) {
    return uuid_is_null(uuid->native_uuid);
}

void sentry_uuid_as_bytes(const sentry_uuid_t *uuid, char bytes[16]) {
    memcpy(bytes, uuid, 16);
}

void sentry_uuid_as_string(const sentry_uuid_t *uuid, char str[37]) {
    uuid_unparse_lower(uuid->native_uuid, str);
}

#endif
