#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <cmocka.h>

#define CONCAT(A, B) A##B
#define SENTRY_TEST(Name) void CONCAT(test_sentry_, Name)(void **state)

#define assert_json_value(Val, ReferenceJson)                                  \
    do {                                                                       \
        char *json = sentry_value_to_json(Val);                                \
        assert_string_equal(json, ReferenceJson);                              \
        sentry_free(json);                                                     \
    } while (0)
