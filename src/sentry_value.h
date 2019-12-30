#ifndef SENTRY_VALUE_H_INCLUDED
#define SENTRY_VALUE_H_INCLUDED

#include <sentry.h>

sentry_value_t sentry__value_new_string_owned(char *s);
sentry_value_t sentry__value_new_addr(uint64_t addr);
sentry_value_t sentry__value_new_hexstring(const char *bytes, size_t len);
sentry_value_t sentry__value_new_uuid(const sentry_uuid_t *uuid);
sentry_value_t sentry__value_new_level(sentry_level_t level);

#endif