#include "sentry_symbolizer.h"
#include "sentry_testsupport.h"
#include <sentry.h>

#define MAX_FRAMES 128

TEST_VISIBLE void **
invoke_unwinder(void)
{
    void **backtrace = sentry_malloc(sizeof(void *) * MAX_FRAMES);
    memset(backtrace, 0, sizeof(void *) * MAX_FRAMES);

    // capture a stacktrace from within this function, which means that the
    // trace will include a frame of this very function that we will test
    // against.
    size_t frame_count = sentry_unwind_stack(NULL, backtrace, MAX_FRAMES);
    TEST_CHECK(frame_count > 0);

    return backtrace;
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
    void **backtrace = invoke_unwinder();

    int found_frame = 0;
    for (int i = 0; i < MAX_FRAMES && backtrace[i]; i++) {
        // symbolize the frame, which also resolves an arbitrary instruction
        // address back to a function, and check if we found our invoker
        sentry__symbolize(backtrace[i], find_frame, &found_frame);
    }
    sentry_free(backtrace);

    TEST_CHECK_INT_EQUAL(found_frame, 1);
}
