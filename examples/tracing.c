#include <printf.h>
#include <sentry.h>
#include <unistd.h>

void
render_output()
{
    sentry_start_scoped_span("render_output", NULL);
    sleep(1);
    sentry_finish();
}
void
process_data()
{
    sentry_start_scoped_span("process_data", NULL);

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "my-logger", "Just FYI in process_data!");
    sentry_capture_event(event);
    sentry_value_decref(event);

    sleep(3);
    event = sentry_value_new_message_event(
        SENTRY_LEVEL_WARNING, "my-logger", "Something weird in process_data!");
    sentry_capture_event(event);
    printf(
        "\nprocess_data, before decref: %zu\n", sentry_value_refcount(event));
    sentry_value_decref(event);
    printf("process_data, after decref: %zu\n", sentry_value_refcount(event));

    sentry_finish();
}
void
request_from_db()
{
    sentry_start_scoped_span("request_from_db", NULL);

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "my-logger", "Just FYI in request_from_db!");
    sentry_capture_event(event);
    sentry_value_decref(event);
    sleep(2);

    sentry_finish();
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

    sentry_value_t event_after_init = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "my-logger", "Right after init");
    sentry_capture_event(event_after_init);
    printf("\nmain, after init, before decref: %zu\n",
        sentry_value_refcount(event_after_init));
    sentry_value_decref(event_after_init);
    printf("main, after init, after decref: %zu\n",
        sentry_value_refcount(event_after_init));

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
    printf("\nmain, before decref: %zu\n", sentry_value_refcount(event));
    sentry_value_decref(event);
    printf("main, after decref: %zu\n", sentry_value_refcount(event));

    // span
    request_from_db();

    // span
    process_data();

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