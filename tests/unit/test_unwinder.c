#include "sentry_scope.h"
#include "sentry_symbolizer.h"
#include "sentry_testsupport.h"
#include <sentry.h>

#define MAX_FRAMES 128

TEST_VISIBLE size_t
invoke_unwinder(void **backtrace)
{
    // capture a stacktrace from within this function, which means that the
    // trace will include a frame of this very function that we will test
    // against.
    size_t frame_count = sentry_unwind_stack(NULL, backtrace, MAX_FRAMES);
    // this assertion here makes sure the function is not inlined.
    TEST_CHECK(frame_count > 0);
    return frame_count;
}

static void
find_frame(const sentry_frame_info_t *info, void *data)
{
    int *found_frame = data;
    // we will specifically check for the unwinder function
    if (info->symbol_addr == &invoke_unwinder) {
        *found_frame += 1;
    }
}

SENTRY_TEST(unwinder)
{
    void *backtrace1[MAX_FRAMES] = { 0 };
    size_t frame_count1 = invoke_unwinder(backtrace1);
    size_t invoker_frame = 0;

    int found_frame = 0;
    for (size_t i = 0; i < frame_count1; i++) {
        // symbolize the frame, which also resolves an arbitrary instruction
        // address back to a function, and check if we found our invoker
        sentry__symbolize(backtrace1[i], find_frame, &found_frame);
        if (found_frame) {
            invoker_frame = i;
        }
    }

    TEST_CHECK_INT_EQUAL(found_frame, 1);

    // the backtrace should have:
    // X. something internal to sentry and the unwinder
    // 2. the `invoke_unwinder` function
    // 3. this test function here
    // 4. whatever parent called that test function, which should have a stable
    //    instruction pointer as long as we don’t return from here.
    size_t offset = invoker_frame + 2;
    if (frame_count1 >= offset) {
        void *backtrace2[MAX_FRAMES] = { 0 };
        size_t frame_count2
            = sentry_unwind_stack(backtrace1[offset], backtrace2, MAX_FRAMES);

        // we only have this functionality on some platforms / unwinders
        if (frame_count2 > 0) {
            TEST_CHECK_INT_EQUAL(frame_count2, frame_count1 - offset);
            for (size_t i = 0; i < frame_count2; i++) {
                TEST_CHECK_INT_EQUAL(backtrace2[i], backtrace1[offset + i]);
            }
        }
    }
}

TEST_VISIBLE sentry_value_t
capture_inapp_event()
{
    sentry_value_t event = sentry_value_new_event();
    sentry_event_value_add_stacktrace(event, NULL, 0);
    return event;
}

SENTRY_TEST(inapp_stacktrace)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_add_in_app_include(options, "capture_inapp_event");

    sentry_value_t event = capture_inapp_event();
    SENTRY_WITH_SCOPE (scope) {
        // this will symbolize all the stacktraces, and flag `in_app`.
        sentry__scope_apply_to_event(scope, options, event, SENTRY_SCOPE_ALL);
    }

    size_t in_app = 0;
    bool found = false;

    sentry_value_t threads = sentry_value_get_by_key(event, "threads");
    sentry_value_t values = sentry_value_get_by_key(threads, "values");
    sentry_value_t thread = sentry_value_get_by_index(values, 0);
    sentry_value_t stacktrace = sentry_value_get_by_key(thread, "stacktrace");
    sentry_value_t frames = sentry_value_get_by_key(stacktrace, "frames");

    size_t len = sentry_value_get_length(frames);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t frame = sentry_value_get_by_index(frames, i);

        const char *symbol = sentry_value_as_string(
            sentry_value_get_by_key(frame, "function"));
        bool is_in_app
            = sentry_value_is_true(sentry_value_get_by_key(frame, "in_app"));
        if (strcmp(symbol, "capture_inapp_event") == 0) {
            found = true;
            TEST_CHECK(is_in_app);
        }

        in_app += is_in_app;
    }

    TEST_CHECK(found);
    TEST_CHECK_INT_EQUAL(in_app, 1);

    sentry_options_free(options);
    sentry_value_decref(event);
    sentry__scope_cleanup();
}
