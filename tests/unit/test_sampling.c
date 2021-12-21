#include "sentry_core.h"
#include "sentry_testsupport.h"

SENTRY_TEST(sampling_decision)
{
    TEST_CHECK(sentry__roll_dice(0.0) == false);
    TEST_CHECK(sentry__roll_dice(1.0));
    TEST_CHECK(sentry__roll_dice(2.0));
}

SENTRY_TEST(sampling_transaction)
{
    sentry_options_t *options = sentry_options_new();
    TEST_CHECK(sentry_init(options) == 0);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("honk", NULL);

    sentry_transaction_context_set_sampled(tx_cxt, 0);
    TEST_CHECK(sentry__should_send_transaction(tx_cxt) == false);

    sentry_transaction_context_set_sampled(tx_cxt, 1);
    TEST_CHECK(sentry__should_send_transaction(tx_cxt));

    // fall back to default in sentry options (0.0) if sampled isn't there
    sentry_transaction_context_remove_sampled(tx_cxt);
    TEST_CHECK(sentry__should_send_transaction(tx_cxt) == false);

    options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    TEST_CHECK(sentry_init(options) == 0);

    TEST_CHECK(sentry__should_send_transaction(tx_cxt));

    sentry_value_decref(tx_cxt);
    sentry_close();
}
