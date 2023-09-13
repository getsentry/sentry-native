#include "sentry_slice.h"
#include "sentry_testsupport.h"

SENTRY_TEST(slice)
{
    char buf[] = "my string buffer";
    char sbuf[] = "string";

    // we don’t have explicit slicing functions, so create the slices manually
    sentry_slice_t my = { buf, 2 };
    sentry_slice_t str1 = { buf + 3, 6 };
    sentry_slice_t str2 = { sbuf, 6 };

    TEST_CHECK(sentry__slice_eq(str1, str2));
    TEST_CHECK(!sentry__slice_eq(str1, my));

    char *owned = sentry__slice_to_owned(str1);
    TEST_CHECK_STRING_EQUAL(owned, "string");
    sentry_free(owned);
}

SENTRY_TEST(null_slices)
{

    char buf[] = "my string buffer";
    sentry_slice_t valid_slice = { buf, sizeof(buf) - 1 };
    sentry_slice_t null_slice = { NULL, 100 };
    TEST_CHECK(!sentry__slice_eq(valid_slice, null_slice));
    TEST_CHECK(!sentry__slice_eq(null_slice, valid_slice));
    TEST_CHECK(!sentry__slice_eq(null_slice, null_slice));
    TEST_CHECK(!sentry__slice_eqs(null_slice, buf));
}
