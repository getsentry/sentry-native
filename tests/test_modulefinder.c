#include "../src/sentry_modulefinder.h"
#include "sentry_testsupport.h"
#include <sentry.h>

SENTRY_TEST(test_module_finder)
{
    sentry_value_t modules = sentry__modules_get_list();
    TEST_CHECK(sentry_value_get_length(modules) > 0);

    bool found_test = false;
    for (size_t i = 0; i < sentry_value_get_length(modules); i++) {
        sentry_value_t mod = sentry_value_get_by_index(modules, i);
        sentry_value_t name = sentry_value_get_by_key(mod, "code_file");
        const char *name_str = sentry_value_as_string(name);
        if (strstr(name_str, "sentry_tests")) {
            found_test = true;
        }
    }

    TEST_CHECK(found_test);
}