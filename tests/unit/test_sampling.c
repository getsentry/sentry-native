#include "sentry_sampling_context.h"
#include "sentry_testsupport.h"
#include "sentry_tracing.h"

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

    sentry_transaction_context_t *tx_cxt
        = sentry_transaction_context_new("honk", NULL);

    sentry_transaction_context_set_sampled(tx_cxt, 0);
    sentry_sampling_context_t sampling_ctx
        = { NULL, sentry_value_new_null(), sentry_value_new_null() };
    TEST_CHECK(
        sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx) == false);

    sentry_transaction_context_set_sampled(tx_cxt, 1);
    TEST_CHECK(sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx));

    // fall back to default in sentry options (0.0) if sampled isn't there
    sentry_transaction_context_remove_sampled(tx_cxt);
    TEST_CHECK(
        sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx) == false);

    options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    TEST_CHECK(sentry_init(options) == 0);

    TEST_CHECK(sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx));

    sentry__transaction_context_free(tx_cxt);
    sentry_close();
}
