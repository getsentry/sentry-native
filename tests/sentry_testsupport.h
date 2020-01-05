#include "../src/sentry_boot.h"

#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef SENTRY_TEST_DEFINE_MAIN
#    define TEST_NO_MAIN
#endif
#include "../vendor/acutest.h"

#define CONCAT(A, B) A##B
#define SENTRY_TEST(Name) void CONCAT(test_sentry_, Name)(void **state)

#define ASSERT_STRING_EQUAL(Val, ReferenceJson)                                \
    do {                                                                       \
        TEST_ASSERT(strcmp(Val, ReferenceJson) == 0);                          \
    } while (0)

#define ASSERT_JSON_VALUE(Val, ReferenceJson)                                  \
    do {                                                                       \
        char *json = sentry_value_to_json(Val);                                \
        ASSERT_STRING_EQUAL(json, ReferenceJson);                              \
        sentry_free(json);                                                     \
    } while (0)

#define ASSERT_INT_EQUAL(A, B)                                                 \
    do {                                                                       \
        int _a = A;                                                            \
        int _b = B;                                                            \
        TEST_ASSERT_(_a == _b, "%d == %d", _a, _b);                            \
    } while (0)