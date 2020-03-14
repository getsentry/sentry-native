#ifndef SENTRY_VALUE_H_INCLUDED
#define SENTRY_VALUE_H_INCLUDED

#include "sentry_boot.h"

sentry_value_t sentry__value_new_string_owned(char *s);
#ifdef SENTRY_PLATFORM_WINDOWS
sentry_value_t sentry__value_new_string_from_wstr(const wchar_t *s);
#endif
sentry_value_t sentry__value_new_addr(uint64_t addr);
sentry_value_t sentry__value_new_hexstring(const char *bytes, size_t len);
sentry_value_t sentry__value_new_uuid(const sentry_uuid_t *uuid);
sentry_value_t sentry__value_new_level(sentry_level_t level);

sentry_value_t sentry__value_new_list_with_size(size_t size);
sentry_value_t sentry__value_new_object_with_size(size_t size);

/* performs a shallow clone. On a frozen value this produces an unfrozen one */
sentry_value_t sentry__value_clone(sentry_value_t value);
int sentry__value_append_bounded(
    sentry_value_t value, sentry_value_t v, size_t max);

// this is actually declared in sentry_json.h
sentry_value_t sentry__value_from_json(const char *buf, size_t buflen);

#endif
