#include "sentry_random.h"
#include <sentry.h>
#include <stdio.h>
#include <string.h>

sentry_uuid_t
sentry_uuid_nil()
{
    sentry_uuid_t rv;
    memset(rv.bytes, 0, 16);
    return rv;
}

sentry_uuid_t
sentry_uuid_new_v4()
{
    char buf[16];
    if (sentry__getrandom(buf, sizeof(buf)) != 0) {
        return sentry_uuid_nil();
    }
    buf[6] = (buf[6] & 0x0f) | 0x40;
    return sentry_uuid_from_bytes(buf);
}

sentry_uuid_t
sentry_uuid_from_string(const char *str)
{
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
            rv.bytes[pos++] = (nibble << 4) | val;
            is_nibble = true;
        }
    }

    return rv;
}

sentry_uuid_t
sentry_uuid_from_bytes(const char bytes[16])
{
    sentry_uuid_t rv;
    memcpy(rv.bytes, bytes, 16);
    return rv;
}

int
sentry_uuid_is_nil(const sentry_uuid_t *uuid)
{
    for (size_t i = 0; i < 16; i++) {
        if (uuid->bytes[i]) {
            return false;
        }
    }
    return true;
}

void
sentry_uuid_as_bytes(const sentry_uuid_t *uuid, char bytes[16])
{
    memcpy(bytes, uuid->bytes, 16);
}

void
sentry_uuid_as_string(const sentry_uuid_t *uuid, char str[37])
{
    snprintf(str, 37,
        "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%"
        "02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        (unsigned char)uuid->bytes[0], (unsigned char)uuid->bytes[1],
        (unsigned char)uuid->bytes[2], (unsigned char)uuid->bytes[3],
        (unsigned char)uuid->bytes[4], (unsigned char)uuid->bytes[5],
        (unsigned char)uuid->bytes[6], (unsigned char)uuid->bytes[7],
        (unsigned char)uuid->bytes[8], (unsigned char)uuid->bytes[9],
        (unsigned char)uuid->bytes[10], (unsigned char)uuid->bytes[11],
        (unsigned char)uuid->bytes[12], (unsigned char)uuid->bytes[13],
        (unsigned char)uuid->bytes[14], (unsigned char)uuid->bytes[15]);
}