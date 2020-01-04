#include "sentry_testsupport.h"

#define XX(Name) void CONCAT(test_sentry_, Name)(void **state);
#include "tests.inc"
#undef XX

static const struct CMUnitTest tests[] = {
#define DECLARE_TEST(Name) cmocka_unit_test(Name)
#define XX(Name) DECLARE_TEST(CONCAT(test_sentry_, Name)),
#include "tests.inc"
#undef XX
#undef DECLARE_TEST
};

int
main(void)
{
    return cmocka_run_group_tests(tests, NULL, NULL);
}
