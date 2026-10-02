#include "sentry.h"
#include <stdlib.h>

static sentry_value_t
before_transaction(sentry_value_t transaction, void *user_data)
{
    (void)user_data;

    sentry_value_set_by_key(
        transaction, "transaction", sentry_value_new_string("resource.load"));
    return transaction;
}

int
main(void)
{
    sentry_options_t *options = sentry_options_new();
    // sentry_options_set_dsn(options, "https://KEY@oORG.ingest.sentry.io/PRJ");
    sentry_options_set_debug(options, 1);
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_before_transaction(options, before_transaction, NULL);

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    sentry_transaction_context_t *context
        = sentry_transaction_context_new("resource.load.config", "task");
    sentry_transaction_t *transaction
        = sentry_transaction_start(context, sentry_value_new_null());
    sentry_transaction_set_tag(transaction, "resource.type", "configuration");
    sentry_transaction_set_data(
        transaction, "resource.name", sentry_value_new_string("config.json"));

    sentry_span_t *read = sentry_transaction_start_child(
        transaction, "resource.read", "Read resource");
    sentry_span_finish(read);

    sentry_span_t *parse = sentry_transaction_start_child(
        transaction, "resource.parse", "Parse resource");
    sentry_span_finish(parse);

    sentry_transaction_finish(transaction);
    sentry_close();
}
