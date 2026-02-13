#include "sentry_random.h"
#include "sentry_string.h"
#include "sentry_testsupport.h"

void
assert_addr_value_equals_format(uint64_t addr)
{
    char our_buf[32];
    char stdio_buf[32];
    sentry__addr_to_string(our_buf, sizeof(our_buf), addr);
    snprintf(stdio_buf, sizeof(stdio_buf), "0x%" PRIx64, addr);
    TEST_CHECK_STRING_EQUAL(our_buf, stdio_buf);
}

SENTRY_TEST(string_address_format)
{
    assert_addr_value_equals_format(0);
    assert_addr_value_equals_format(0xf000000000000000);
    assert_addr_value_equals_format(0x000000000000000f);
    assert_addr_value_equals_format(0x0000ffff0000ffff);
    assert_addr_value_equals_format(0x0000ffffffff0000);
    assert_addr_value_equals_format(0xf0f0f0f0f0f0f0f0);
    assert_addr_value_equals_format(0x0f0f0f0f0f0f0f0f);
    uint64_t rnd;
    for (int i = 0; i < 1000000; ++i) {
        TEST_ASSERT(!sentry__getrandom(&rnd, sizeof(rnd)));
        assert_addr_value_equals_format(rnd);
    }
    assert_addr_value_equals_format(0xffffffffffffffff);
}
