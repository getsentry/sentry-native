#include "../src/sentry_symbolizer.h"
#include "sentry_testsupport.h"
#include <sentry.h>

// NOTE: marking this as `SENTRY_API` will make the symbol public on linux
// in order for `dladdr` to correctly find its name and offset
SENTRY_API void
test_function()
{
    printf("Something here\n");
}

static void
asserter(const sentry_frame_info_t *info, void *data)
{
    int *called = data;
    TEST_CHECK(!!info->symbol);
    TEST_CHECK(strstr(info->symbol, "test_function") != 0);
    TEST_CHECK(strstr(info->object_name, "sentry_tests") != 0);
    TEST_CHECK(info->symbol_addr == &test_function);
    TEST_CHECK(info->instruction_addr == ((char *)(void *)&test_function) + 1);
    *called += 1;
}

SENTRY_TEST(test_symbolizer)
{
    int called = 0;
    sentry__symbolize(((char *)(void *)&test_function) + 1, asserter, &called);
    TEST_CHECK_INT_EQUAL(called, 1);
}
