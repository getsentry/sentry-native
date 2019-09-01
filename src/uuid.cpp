#include "uuid.hpp"

#ifdef _WIN32
sentry_uuid_t sentry_uuid_nil() {
    sentry_uuid_t rv;
    UuidCreateNil(&rv.native_uuid);
    return rv;
}

sentry_uuid_t sentry_uuid_new_v4() {
    sentry_uuid_t rv;
    UuidCreate(&rv.native_uuid);
    return rv;
}

sentry_uuid_t sentry_uuid_from_string(const char *str) {
    sentry_uuid_t rv;
    if (UuidFromStringA((RPC_CSTR)str, &rv.native_uuid) != RPC_S_OK) {
        UuidCreateNil(&rv.native_uuid);
    }
    return rv;
}

sentry_uuid_t sentry_uuid_from_bytes(const char bytes[16]) {
    sentry_uuid_t rv;
    char *uuid_bytes = (char *)&rv.native_uuid;
    memcpy(uuid_bytes, bytes, 16);
    std::reverse(uuid_bytes, uuid_bytes + 4);
    std::reverse(uuid_bytes + 4, uuid_bytes + 6);
    std::reverse(uuid_bytes + 6, uuid_bytes + 8);
    return rv;
}

int sentry_uuid_is_nil(const sentry_uuid_t *uuid) {
    RPC_STATUS status;
    return !!UuidIsNil((UUID *)&uuid->native_uuid, &status);
}

void sentry_uuid_as_bytes(const sentry_uuid_t *uuid, char bytes[16]) {
    memcpy(bytes, uuid, 16);
    std::reverse(bytes, bytes + 4);
    std::reverse(bytes + 4, bytes + 6);
    std::reverse(bytes + 6, bytes + 8);
}

void sentry_uuid_as_string(const sentry_uuid_t *uuid, char str[37]) {
    unsigned char *out = 0;
    UuidToStringA(&uuid->native_uuid, &out);
    memcpy(str, (const char *)out, 37);
    RpcStringFreeA(&out);
}
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

sentry_uuid_t sentry_uuid_from_bytes(const char bytes[16]) {
    sentry_uuid_t rv;
    memcpy(rv.native_uuid, bytes, 16);
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
