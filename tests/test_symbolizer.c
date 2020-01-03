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
    assert_string_equal(info->symbol, "test_function");
    assert_true(strstr(info->object_name, "sentry_tests"));
    assert_int_equal(info->symbol_addr, &test_function);
    assert_int_equal(info->instruction_addr, (void *)&test_function + 1);
    *called += 1;
}

SENTRY_TEST(test_symbolizer)
{
    int called = 0;
    sentry__symbolize((void *)&test_function + 1, asserter, &called);
    assert_int_equal(called, 1);
}