#include "sentry_core.h"
#include "sentry_testsupport.h"

SENTRY_TEST(sampling_decision)
{
    TEST_CHECK(sentry__is_unsampled(0.0));
    TEST_CHECK(sentry__is_unsampled(1.0) == false);
    TEST_CHECK(sentry__is_unsampled(2.0) == false);
}

SENTRY_TEST(sampling_transaction)
{
    sentry_options_t *options = sentry_options_new();
    TEST_CHECK(sentry_init(options) == 0);

    // TODO: replace with proper construction of a transaction, e.g.
    // new_transaction_context -> transaction_context_set_sampled ->
    // start_transaction
    // using transaction context in place of a full transaction for now.
    sentry_value_t tx_cxt = sentry_value_new_transaction_context("honk");

    sentry_transaction_context_set_sampled(tx_cxt, 0);
    TEST_CHECK(sentry__should_skip_transaction(tx_cxt));

    sentry_transaction_context_set_sampled(tx_cxt, 1);
    TEST_CHECK(sentry__should_skip_transaction(tx_cxt) == false);

    // fall back to default in sentry options (0.0) if sampled isn't there
    sentry_transaction_context_remove_sampled(tx_cxt);
    TEST_CHECK(sentry__should_skip_transaction(tx_cxt));

    options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    TEST_CHECK(sentry_init(options) == 0);

    TEST_CHECK(sentry__should_skip_transaction(tx_cxt) == false);

    sentry_value_decref(tx_cxt);
}

SENTRY_TEST(sampling_event)
{
    // default is to sample all (error) events, and to not sample any
    // transactions
    sentry_options_t *options = sentry_options_new();

    sentry_value_t event = sentry_value_new_object();
    sentry_value_set_by_key(event, "sampled", sentry_value_new_bool(0));

    // events ignore sampled field if they're not transactions
    TEST_CHECK(sentry__should_skip_event(options, event) == false);

    // respect sampled field if it is a transaction
    sentry_value_set_by_key(
        event, "type", sentry_value_new_string("transaction"));
    TEST_CHECK(sentry__should_skip_event(options, event));

    // if the sampled field isn't set on a transaction, don't ever send
    // transactions even if the option says to do so
    sentry_value_remove_by_key(event, "sampled");
    TEST_CHECK(sentry__should_skip_event(options, event));
    sentry_options_set_traces_sample_rate(options, 1.0);
    TEST_CHECK(sentry__should_skip_event(options, event));

    sentry_value_decref(event);
    sentry_options_free(options);
}
