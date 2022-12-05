#include <sentry.h>
#include <unistd.h>

#define WRAP_IN_SPAN(OP, DESC)                                                 \
    sentry_start_scoped_span(OP, DESC);                                        \
    for (int i = 0; i < 1; i++, sentry_finish())

void
render_output()
{
    WRAP_IN_SPAN("render_output", NULL) { sleep(1); }
}
void
process_data()
{
    WRAP_IN_SPAN("process_data", NULL)
    {
        sentry_capture_event(sentry_value_new_message_event(
            SENTRY_LEVEL_INFO, "my-logger", "Just FYI in process_data!"));

        sleep(3);

        sentry_capture_event(
            sentry_value_new_message_event(SENTRY_LEVEL_WARNING, "my-logger",
                "Something weird in process_data!"));
    }
}
void
request_from_db()
{
    WRAP_IN_SPAN("request_from_db", NULL)
    {
        sentry_capture_event(sentry_value_new_message_event(
            SENTRY_LEVEL_INFO, "my-logger", "Just FYI in request_from_db!"));
        sleep(2);
    }
}

int
main()
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, 1);
    sentry_options_set_dsn(options,
        "https://baf6fb0ee9aa488ab2ca88289ee61d38@o1250814.ingest.sentry.io/"
        "6415577");
    sentry_options_set_handler_path(options,
        "/Users/mischan/devel/sentry-native/cmake-build-relwithdebinfo/"
        "crashpad_build/handler/crashpad_handler");
    sentry_options_set_database_path(
        options, "/Users/mischan/devel/tmp/tracing");
    // XXX: the default is 0!!! so we must do something about it.
    // XXX: also: this doesn't have to be set, if you set the traces sample rate
    // sentry_options_set_max_spans(options, 32);
    // XXX: if you don't want to sample traces then you need to set the rate
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_init(options);

    // since we are not being called from anywhere and thus have not received
    // a parent span, we need to create an explicit transaction start
    sentry_transaction_context_t *txn_ctx
        = sentry_transaction_context_new("renderer", "renderer");

    // XXX: super important: while you can set this to 0 to not sample it, not
    // setting it, doesn't enable it either, because not setting it means the
    // dice will be rolled which depends on traces_sample_rate is set to 0.0 by
    // default... i.e. you either have to not set it and leave it up to the
    // dice, but then configure above in the options the probability of the
    // dice. Or you set it, where setting it to 0 will send no transaction or
    // child ever and to 1 will send every transaction and child.
    //    sentry_transaction_context_set_sampled(txn_ctx, 1);

    sentry_transaction_t *txn
        = sentry_transaction_start(txn_ctx, sentry_value_new_null());
    sentry_set_transaction_object(txn);

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "my-logger", "Welcome to the renderer!");
    sentry_capture_event(event);

    // span
    request_from_db();
    sentry_set_transaction_object(txn);

    // span
    process_data();
    sentry_set_transaction_object(txn);

    // span
    render_output();
    sentry_set_transaction_object(txn);

    // explicit transaction end
    sentry_transaction_finish(txn);

    sentry_close();

    return 0;
}

/**
 * -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
 */