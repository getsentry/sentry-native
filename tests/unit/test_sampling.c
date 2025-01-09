#include "sentry_options.h"
#include "sentry_sampling_context.h"
#include "sentry_testsupport.h"
#include "sentry_tracing.h"

SENTRY_TEST(sampling_decision)
{
    TEST_CHECK(sentry__roll_dice(0.0) == false);
    TEST_CHECK(sentry__roll_dice(1.0));
    TEST_CHECK(sentry__roll_dice(2.0));
}

static double
traces_sampler_callback(sentry_sampling_context_t *sampling_ctx)
{
    if (!sentry_value_is_null(
            sentry_sampling_context_get_parent_sampled(sampling_ctx))) {
        if (sentry_value_is_true(
                sentry_sampling_context_get_parent_sampled(sampling_ctx))) {
            return 1; // high sample rate for children of sampled transactions
        }
        return 0; // parent is not sampled
    }
    if (sentry_value_as_int32(sentry_value_get_by_key(
            sentry_sampling_context_get_custom_context(sampling_ctx), "answer"))
        == 42) {
        return 1;
    }
    return 0;
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

    // sampled parent -> sampled child
    sentry_transaction_context_update_from_header(tx_cxt, "sentry-trace",
        "12345678901234567890123456789012-1234567890123456-1");
    TEST_CHECK(sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx));

    options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    TEST_CHECK(sentry_init(options) == 0);

    TEST_CHECK(sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx));

    // non-sampled parent
    sentry_transaction_context_update_from_header(tx_cxt, "sentry-trace",
        "12345678901234567890123456789012-1234567890123456-0");
    TEST_CHECK(
        sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx) == false);

    sentry_transaction_context_remove_sampled(tx_cxt);

    // test the traces_sampler callback
    options = sentry_options_new();
    sentry_options_set_traces_sampler(options, traces_sampler_callback);
    sentry_options_set_traces_sample_rate(options, 1.0);
    TEST_CHECK(sentry_init(options) == 0);

    sentry_value_t custom_sampling_ctx = sentry_value_new_object();
    sentry_value_set_by_key(
        custom_sampling_ctx, "answer", sentry_value_new_int32(42));
    sampling_ctx.custom_sampling_context = custom_sampling_ctx;

    TEST_CHECK(sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx));

    // non-sampled parent and traces sampler
    sentry_transaction_context_update_from_header(tx_cxt, "sentry-trace",
        "12345678901234567890123456789012-1234567890123456-0");
    TEST_CHECK(
        sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx) == false);
    // removing sampled should fall back to traces sampler
    sentry_transaction_context_remove_sampled(tx_cxt);
    TEST_CHECK(sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx));
    sentry_value_set_by_key(
        custom_sampling_ctx, "answer", sentry_value_new_int32(21));
    TEST_CHECK(
        sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx) == false);

    // sampled parent and traces sampler
    sentry_transaction_context_update_from_header(tx_cxt, "sentry-trace",
        "12345678901234567890123456789012-1234567890123456-1");
    TEST_CHECK(sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx));
    sentry_transaction_context_remove_sampled(tx_cxt);

    // remove traces_sampler callback, should fall back to traces_sample_rate
    options->traces_sampler = NULL;
    sentry_options_set_traces_sample_rate(options, 0.0);
    TEST_CHECK(
        sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx) == false);
    sentry_options_set_traces_sample_rate(options, 1.0);
    TEST_CHECK(sentry__should_send_transaction(tx_cxt->inner, &sampling_ctx));

    sentry__transaction_context_free(tx_cxt);
    sentry_close();
}
