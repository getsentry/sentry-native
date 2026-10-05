#include "sentry_boot.h"

#include <stdlib.h>

sentry_value_t
sentry_lookup_module(const void *address)
{
    if (!address) {
        return sentry_value_new_null();
    }

    uint64_t instruction = (uintptr_t)address;
    sentry_value_t modules = sentry_get_modules_list();
    sentry_value_t result = sentry_value_new_null();
    for (size_t i = 0; i < sentry_value_get_length(modules); i++) {
        sentry_value_t module = sentry_value_get_by_index(modules, i);
        const char *image_address = sentry_value_as_string(
            sentry_value_get_by_key(module, "image_addr"));
        if (!image_address[0]) {
            continue;
        }
        uint64_t start = strtoull(image_address, NULL, 0);
        int64_t size = sentry_value_as_int64(
            sentry_value_get_by_key(module, "image_size"));
        if (size > 0 && instruction >= start
            && instruction - start < (uint64_t)size) {
            result = sentry_value_incref(module);
            break;
        }
    }
    sentry_value_decref(modules);
    return result;
}
