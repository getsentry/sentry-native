#include "../src/sentry_symbolizer.h"
#include "sentry_testsupport.h"
#include <sentry.h>

void
test_function()
{
    printf("Something here\n");
}

static void
asserter(const sentry_frame_info_t *info, void *data)
{
    int *called = data;
    TEST_ASSERT(!!info->symbol);
    ASSERT_STRING_EQUAL(info->symbol, "test_function");
    TEST_ASSERT(!!info->object_name);
    TEST_ASSERT(strstr(info->object_name, "sentry_tests") != 0);
    TEST_ASSERT(info->symbol_addr == &test_function);
    TEST_ASSERT(info->instruction_addr == (void *)&test_function + 1);
    *called += 1;
}

SENTRY_TEST(test_symbolizer)
{
    int called = 0;
    sentry__symbolize((void *)&test_function + 1, asserter, &called);
    ASSERT_INT_EQUAL(called, 1);
}