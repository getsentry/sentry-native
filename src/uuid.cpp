#include <cstdio>

#include "uuid.hpp"

#if defined(SENTRY_UUID_WINDOWS)
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
#elif defined(SENTRY_UUID_ANDROID)
sentry_uuid_t sentry_uuid_nil() {
    sentry_uuid_t rv;
    memset(rv.native_uuid, 0, 16);
    return rv;
}

sentry_uuid_t sentry_uuid_new_v4() {
    /* in theory we can read /proc/sys/kernel/random/uuid but on some
       devices this seems to give a permission error */
    FILE *fd = fopen("/dev/urandom", "r");
    if (!fd) {
        return sentry_uuid_nil();
    }
    char buf[16];
    size_t read = fread(buf, 1, sizeof(buf), fd);
    if (read != sizeof(buf)) {
        return sentry_uuid_nil();
    }
    fclose(fd);
    buf[6] = (buf[6] & 0x0f) | 0x40;
    return sentry_uuid_from_bytes(buf);
}

sentry_uuid_t sentry_uuid_from_string(const char *str) {
    sentry_uuid_t rv;

    size_t i = 0;
    size_t len = strlen(str);
    size_t pos = 0;
    bool is_nibble = true;
    char nibble;

    for (i = 0; i < len && pos < 16; i++) {
        char c = str[i];
        if (!c || c == '-') {
            continue;
        }

        char val = 0;
        if (c >= 'a' && c <= 'f') {
            val = 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            val = 10 + (c - 'A');
        } else if (c >= '0' && c <= '9') {
            val = c - '0';
        } else {
            return sentry_uuid_nil();
        }

        if (is_nibble) {
            nibble = val;
            is_nibble = false;
        } else {
            rv.native_uuid[pos++] = (nibble << 4) | val;
            is_nibble = true;
        }
    }

    return rv;
}

sentry_uuid_t sentry_uuid_from_bytes(const char bytes[16]) {
    sentry_uuid_t rv;
    memcpy(rv.native_uuid, bytes, 16);
    return rv;
}

int sentry_uuid_is_nil(const sentry_uuid_t *uuid) {
    for (size_t i = 0; i < 16; i++) {
        if (uuid->native_uuid[i]) {
            return false;
        }
    }
    return true;
}

void sentry_uuid_as_bytes(const sentry_uuid_t *uuid, char bytes[16]) {
    memcpy(bytes, uuid->native_uuid, 16);
}

void sentry_uuid_as_string(const sentry_uuid_t *uuid, char str[37]) {
    snprintf(str, 37,
             "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%"
             "02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
             (unsigned char)uuid->native_uuid[0],
             (unsigned char)uuid->native_uuid[1],
             (unsigned char)uuid->native_uuid[2],
             (unsigned char)uuid->native_uuid[3],
             (unsigned char)uuid->native_uuid[4],
             (unsigned char)uuid->native_uuid[5],
             (unsigned char)uuid->native_uuid[6],
             (unsigned char)uuid->native_uuid[7],
             (unsigned char)uuid->native_uuid[8],
             (unsigned char)uuid->native_uuid[9],
             (unsigned char)uuid->native_uuid[10],
             (unsigned char)uuid->native_uuid[11],
             (unsigned char)uuid->native_uuid[12],
             (unsigned char)uuid->native_uuid[13],
             (unsigned char)uuid->native_uuid[14],
             (unsigned char)uuid->native_uuid[15]);
}

#elif defined(SENTRY_UUID_LIBUUID)
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
